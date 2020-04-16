/******************************************************************************
 *
 * $Id$
 *
 * Project:  WindNinja
 * Purpose:  Semi lagrangian solver
 * Author:   Jason Forthofer
 *
 ******************************************************************************
 *
 * THIS SOFTWARE WAS DEVELOPED AT THE ROCKY MOUNTAIN RESEARCH STATION (RMRS)
 * MISSOULA FIRE SCIENCES LABORATORY BY EMPLOYEES OF THE FEDERAL GOVERNMENT
 * IN THE COURSE OF THEIR OFFICIAL DUTIES. PURSUANT TO TITLE 17 SECTION 105
 * OF THE UNITED STATES CODE, THIS SOFTWARE IS NOT SUBJECT TO COPYRIGHT
 * PROTECTION AND IS IN THE PUBLIC DOMAIN. RMRS MISSOULA FIRE SCIENCES
 * LABORATORY ASSUMES NO RESPONSIBILITY WHATSOEVER FOR ITS USE BY OTHER
 * PARTIES,  AND MAKES NO GUARANTEES, EXPRESSED OR IMPLIED, ABOUT ITS QUALITY,
 * RELIABILITY, OR ANY OTHER CHARACTERISTIC.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#include "ninjaSemiLagrangian.h"

NinjaSemiLagrangian::NinjaSemiLagrangian() : ninja()
{

}

/**
 * Copy constructor.
 * @param A Copied value.
 */

NinjaSemiLagrangian::NinjaSemiLagrangian(NinjaSemiLagrangian const& A ) : ninja(A), U00(A.U00), transport(A.transport)
{

}

/**
 * Equals operator.
 * @param A Value to set equal to.
 * @return a copy of an object
 */

NinjaSemiLagrangian& NinjaSemiLagrangian::operator= (NinjaSemiLagrangian const& A)
{
    if(&A != this) {
        ninja::operator=(A);
        U00 = A.U00;
        transport = A.transport;
    }
    return *this;
}

NinjaSemiLagrangian::~NinjaSemiLagrangian()
{
    deleteDynamicMemory();
}

/**Method to start a wind simulation.
 * This is the method used to start the "number crunching" part of a simulation.
 * WindNinjaInputs should be completely filled in before running this method.
 * @return Returns true if simulation completes without error.
 */
bool NinjaSemiLagrangian::simulate_wind()
{
    checkCancel();

    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Reading elevation file...");

    readInputFile();
    set_position();
    set_uniVegetation();

    checkInputs();

    if(!input.ninjaTime.is_not_a_date_time())
    {
        std::ostringstream out;
        out << "Simulation time is " << input.ninjaTime;
        input.Com->ninjaCom(ninjaComClass::ninjaNone, out.str().c_str());
    }

#ifdef _OPENMP
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Run number %d started with %d threads.",
            input.inputsRunNumber, input.numberCPUs);
#endif

#ifdef _OPENMP
    startTotal = omp_get_wtime();
#endif

/*  ----------------------------------------*/
/*  USER INPUTS                             */
/*  ----------------------------------------*/
    int MAXITS = 100000;             //MAXITS is the maximum number of iterations in the solver
    double stop_tol = 1E-1;          //stopping criteria for iterations (2-norm of residual)
    int print_iters = 10;          //Iterations to print out

    /*
    ** Set matching its from config options, default to 150.
    ** See constructor to set default.
    */
    //maximum number of outer iterations to do (for matching observations)
    int max_matching_iters = nMaxMatchingIters;

/*  ----------------------------------------*/
/*  MESH GENERATION                         */
/*  ----------------------------------------*/

#ifdef _OPENMP
    startMesh = omp_get_wtime();
#endif

    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Generating mesh...");
    //generate mesh
    mesh.buildStandardMesh(input);

    //u is positive toward East
    //v is positive toward North
    //w is positive up
    U0.allocate(&mesh);
    if(transport.transportType == TransportSemiLagrangian::settls)
        U00.allocate(&mesh);

#ifdef _OPENMP
    endMesh = omp_get_wtime();
#endif

/*  ----------------------------------------*/
/*  START OUTER INTERATIVE LOOP FOR         */
/*  MATCHING INPUT POINTS		    */
/*  ----------------------------------------*/

    if(input.initializationMethod == WindNinjaInputs::pointInitializationFlag)
    {
        if(input.matchWxStations == true)
        {
            input.Com->ninjaCom(ninjaComClass::ninjaNone, "Starting outer wx station \"matching\" loop...");
            //don't print normal solver progress, just "outer iter" "matching" progress
            //If this is commented, it messes with the progress-bar
            input.Com->noSolverProgress();
        }
    }

    int matchingIterCount = 0;
    bool matchFlag = false;
    if(input.matchWxStations == true)
    {
        num_outer_iter_tries_u = std::vector<int>(input.stations.size(),0);
        num_outer_iter_tries_v = std::vector<int>(input.stations.size(),0);
        num_outer_iter_tries_w = std::vector<int>(input.stations.size(),0);
    }

/*  ----------------------------------------*/
/*  VELOCITY INITIALIZATION                 */
/*  ----------------------------------------*/
    do
    {
        if(input.matchWxStations == true)
        {
            matchingIterCount++;
            input.Com->ninjaCom(ninjaComClass::ninjaNone, "\"matching\" loop iteration %i...",
                    matchingIterCount);
        }

#ifdef _OPENMP
        startInit = omp_get_wtime();
#endif

        input.Com->ninjaCom(ninjaComClass::ninjaNone, "Initializing flow...");

        //initialize
        init.reset(initializationFactory::makeInitialization(input));
        init->initializeFields(input, mesh, U0, CloudGrid);
        U00 = U0;


        /////////////Test/////////////////////////////////--------------------------------------------------------------
        volVTK VTK_test(U0, mesh.XORD, mesh.YORD, mesh.ZORD,
        input.dem.get_nCols(), input.dem.get_nRows(), mesh.nlayers, "test.vtk");
        printf("here\n");



        //////////////////////////////////////////////////---------------------------------------------------------------








#ifdef _OPENMP
        endInit = omp_get_wtime();
#endif

        checkCancel();

/*  ----------------------------------------*/
/*  CHECK FOR "NULL" RUN                    */
/*  ----------------------------------------*/
        if(checkForNullRun())	//if it's a run with all zero velocity...
            break;

/*  ----------------------------------------*/
/*  BUILD "A" ARRAY OF AX=B                 */
/*  ----------------------------------------*/
#ifdef _OPENMP
        startBuildEq = omp_get_wtime();
#endif

        input.Com->ninjaCom(ninjaComClass::ninjaNone, "Building equations...");

        //build A arrray
        FEM.SetStability(mesh, input, U0, CloudGrid, init);
        FEM.Discretize(mesh, input, U0);

        checkCancel();

/*  ----------------------------------------*/
/*  SET BOUNDARY CONDITIONS                 */
/*  ----------------------------------------*/

        //set boundary conditions
        FEM.SetBoundaryConditions(mesh, input);

//#define WRITE_A_B
#ifdef WRITE_A_B	//used for debugging...
        FEM.Write_A_and_b(1000);
#endif

#ifdef _OPENMP
        endBuildEq = omp_get_wtime();
#endif

        checkCancel();

/*  ----------------------------------------*/
/*  CALL SOLVER                             */
/*  ----------------------------------------*/

        input.Com->ninjaCom(ninjaComClass::ninjaNone, "Solving...");
#ifdef _OPENMP
        startSolve = omp_get_wtime();
#endif

        //solver
        //if the CG solver diverges, try the minres solver
        if(FEM.Solve(input, mesh.NUMNP, MAXITS, print_iters, stop_tol)==false)
            if(FEM.SolveMinres(input, mesh.NUMNP, MAXITS, print_iters, stop_tol)==false)
                throw std::runtime_error("Solver returned false.");

#ifdef _OPENMP
        endSolve = omp_get_wtime();
#endif

checkCancel();

/*  ----------------------------------------*/
/*  COMPUTE UVW WIND FIELD                   */
/*  ----------------------------------------*/

        //compute uvw field from phi field
        FEM.ComputeUVWField(mesh, input, U0, U);

        checkCancel();

        matchFlag = matched(matchingIterCount);

    }while(matchingIterCount<max_matching_iters && !matchFlag);	//end outer iterations is over max_matching_iters or wind field matches wx stations

    if(input.matchWxStations == true && !isNullRun)
    {
        double smallestInfluenceRadius = getSmallestRadiusOfInfluence();

        if(matchFlag == false)
        {
            const char* error;
            error = CPLSPrintf("Solution did not converge to match weather stations.\n" \
            "Sometimes this is caused by a very low radius of influence when compared to the mesh resolution.\n" \
            "Your horizontal mesh resolution is %lf meters and the smallest radius of influence is %.2E meters,\n" \
            "which means that the radius of influence is %.2E cells in distance.\n" \
            "It is usually a good idea to have at least 10 cells of distance (%.2E meters in this case).\n" \
            "If convergence is still not reached, try increasing the radius of influence even more.\n", \
            mesh.meshResolution, smallestInfluenceRadius,
            smallestInfluenceRadius/mesh.meshResolution, 10.0*mesh.meshResolution);

            input.Com->ninjaCom(ninjaComClass::ninjaWarning, error);
            throw(std::runtime_error(error));
        }
    }

/*  ----------------------------------------*/
/*  PREPARE OUTPUT                          */
/*  ----------------------------------------*/

#ifdef _OPENMP
    startWriteOut = omp_get_wtime();
#endif

    //prepare output arrays
    prepareOutput();

    checkCancel();

/*  ----------------------------------------*/
/*  WRITE OUTPUT FILES                      */
/*  ----------------------------------------*/

    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Writing output files...");

    //write output files
    writeOutputFiles();

#ifdef _OPENMP
    endWriteOut = omp_get_wtime();
    endTotal = omp_get_wtime();
#endif

/*  ----------------------------------------*/
/*  WRAP UP...                              */
/*  ----------------------------------------*/

    //write timers
#ifdef _OPENMP
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Meshing time was %lf seconds.",endMesh-startMesh);
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Initialization time was %lf seconds.",endInit-startInit);
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Equation building time was %lf seconds.",endBuildEq-startBuildEq);
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Solver time was %lf seconds.",endSolve-startSolve);
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Output writing time was %lf seconds.",endWriteOut-startWriteOut);
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Total simulation time was %lf seconds.",endTotal-startTotal);
#endif

     input.Com->ninjaCom(ninjaComClass::ninjaNone, "Run number %d done!", input.inputsRunNumber);

    //If its a pointInitialization Run, explicitly set run completion to 100 when they finish
    //for some reason this doesn't happen automatically
    if(input.initializationMethod == WindNinjaInputs::pointInitializationFlag)
    {
        if(input.matchWxStations == true)
        {
            int time_percent_complete=100;
            input.Com->ninjaCom(ninjaComClass::ninjaOuterIterProgress, "%d",(int) (time_percent_complete+0.5));
        }
    }

    deleteDynamicMemory();
    if(!input.keepOutGridsInMemory)
    {
        AngleGrid.deallocate();
        VelocityGrid.deallocate();
        CloudGrid.deallocate();
    }
    return true;
}

/**Deletes allocated dynamic memory.
 *
 */
void NinjaSemiLagrangian::deleteDynamicMemory()
{
    ninja::deleteDynamicMemory();

    U00.deallocate();
}

