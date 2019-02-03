/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2019 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef SIMPLEX_HAPP_H_
#define SIMPLX_HAPP_H_

// todo: clear includes.
#include <getopt.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "HConfig.h"
#include "HCrash.h"
#include "HDual.h"
#include "HighsLp.h"
#include "HighsModelObject.h"
//#include "HighsModelObjectUtils.h"
#include "HighsUtils.h"
#include "HRanging.h"
#include "HSimplex.h"
#include "SimplexConst.h"

using std::cout;
using std::endl;
using std::flush;

HighsStatus LpStatusToHighsStatus(SimplexSolutionStatus simplex_solution_status) {
  switch (simplex_solution_status) {
  case SimplexSolutionStatus::OUT_OF_TIME:
      return HighsStatus::Timeout;
  case SimplexSolutionStatus::REACHED_DUAL_OBJECTIVE_VALUE_UPPER_BOUND:
      return HighsStatus::ReachedDualObjectiveUpperBound;
  case SimplexSolutionStatus::FAILED:
      return HighsStatus::SolutionError;
  case SimplexSolutionStatus::SINGULAR:
      return HighsStatus::SolutionError;
  case SimplexSolutionStatus::UNBOUNDED:
      return HighsStatus::Unbounded;
  case SimplexSolutionStatus::INFEASIBLE:
      return HighsStatus::Infeasible;
  case SimplexSolutionStatus::OPTIMAL:
      return HighsStatus::Optimal;
  default:
      return HighsStatus::NotImplemented;
  }
}

HighsStatus solveSimplex(
			 const HighsOptions& opt,
                         HighsModelObject& highs_model
			 ) {
  // Just solves the LP in highs_model.scaled_lp_
  HighsTimer &timer = highs_model.timer_;

  HModel& model = highs_model.hmodel_[0];
  HighsSimplexInfo &simplex_info_ = highs_model.simplex_info_;

  timer.start(timer.solveClock);
  bool ranging = true;
  // Initialize solver and set dual solver options from simplex options
  HDual dual_solver(highs_model);
  dual_solver.options();
  
  // If after postsolve. todo: advanced basis start here.
  if (opt.clean_up) {
    model.initFromNonbasic();
    dual_solver.solve();
    return LpStatusToHighsStatus(simplex_info_.solution_status);
  }

  // Crash, if HighsModelObject has basis information.
  if (simplex_info_.crash_strategy != SimplexCrashStrategy::OFF) {
    HCrash crash;
    crash.crash(highs_model, 0);
  }

  // Solve, depending on the options.
  // Parallel.
  if (simplex_info_.simplex_strategy == SimplexStrategy::DUAL_TASKS) {
    dual_solver.solve(8);
  } else if (simplex_info_.simplex_strategy == SimplexStrategy::DUAL_MULTI) {
    //    if (opt.partitionFile.size() > 0) {model.strOption[STROPT_PARTITION_FILE] = opt.partitionFile;}
    dual_solver.solve(8);
#ifdef HiGHSDEV
    // Reinstate this once simplex::writePivots is written
    //    if (simplex_info_.simplex_strategy == SimplexStrategy::DUAL_MULTI) writePivots("multi");
    //    if (simplex_info_.simplex_strategy == SimplexStrategy::DUAL_TASKS) writePivots("tasks");
#endif
  } else {
    // Serial. Based on previous solvePlainJAJH.

    //    double lcSolveTime;

    vector<double> colPrAct;
    vector<double> colDuAct;
    vector<double> rowPrAct;
    vector<double> rowDuAct;

    //  bool FourThreads = true;
    bool FourThreads = false;
    //  bool EightThreads = true;
    bool EightThreads = false;

    if (FourThreads)
      dual_solver.solve(4);
    else if (EightThreads)
      dual_solver.solve(8);
    else
      dual_solver.solve();

#ifdef HiGHSDEV
    double currentRunHighsTime = highs_model.timer_.readRunHighsClock();
    printf(
        "\nBnchmkHsol01 After presolve        ,hsol,%3d,%16s, %d,%d,"
        "%10.3f,%20.10e,%10d,%10d,%10d\n",
        (int) simplex_info_.solution_status,
	highs_model.lp_.model_name_.c_str(),
	highs_model.lp_.numRow_,
        highs_model.lp_.numCol_,
	currentRunHighsTime,
	simplex_info_.dualObjectiveValue,
	simplex_info_.dual_phase1_iteration_count,
        simplex_info_.dual_phase2_iteration_count,
	simplex_info_.primal_phase1_iteration_count);
#endif
    //    reportLp(highs_model.lp_);
    //    reportLpSolution(highs_model);
    HighsStatus result = LpStatusToHighsStatus(simplex_info_.solution_status);

    timer.stop(timer.solveClock);


    if (result != HighsStatus::Optimal) return result;


    // TODO Reinstate this once solve after postsolve is performed
    //  model.util_getPrimalDualValues(colPrAct, colDuAct, rowPrAct, rowDuAct);
    //  double Ph2Objective = model.computePh2Objective(colPrAct);
    //  printf("Computed Phase 2 objective = %g\n", Ph2Objective);

  }
  return HighsStatus::Optimal;
}

HighsStatus solveScip(const HighsOptions& opt, HighsModelObject& highs_model) {
  printf("Called solveScip.\n");

  // This happens locally for now because I am not sure how it is used. Later
  // HModel will disappear and we'll see what the best way is.
  HModel model;
  const HighsLp &lp = highs_model.lp_;

  model.load_fromArrays(lp.numCol_, lp.sense_, &lp.colCost_[0],
                        &lp.colLower_[0], &lp.colUpper_[0], lp.numRow_,
                        &lp.rowLower_[0], &lp.rowUpper_[0], lp.nnz_,
                        &lp.Astart_[0], &lp.Aindex_[0], &lp.Avalue_[0]);


  HSimplex simplex_method_;

  // Extract columns numCol-3..numCol-1
  int FmCol = highs_model.lp_.numCol_ - 3;
  int ToCol = highs_model.lp_.numCol_ - 1;
  int numExtractCols = ToCol - FmCol + 1;
  vector<double> XcolCost;
  vector<double> XcolLower;
  vector<double> XcolUpper;
  vector<int> XAstart;
  vector<int> XAindex;
  vector<double> XAvalue;
  //  model.util_extractCols(FmCol, ToCol, XcolCost, XcolLower, XcolUpper,
  //			 XAstart, XAindex, XAvalue);

  //  printf("Returned from model.util_extractCols with\n");
  //  model.util_reportColVec(numExtractCols, XcolCost, XcolLower, XcolUpper);
  //  model.util_reportColMtx(numExtractCols, XAstart, XAindex, XAvalue);

  // Delete the columns just extracted
  model.util_deleteCols(FmCol, ToCol);
  //  model.util_reportModel();

  // Extract rows numRow-3..numRow-1
  int FmRow = highs_model.lp_.numRow_ - 3;
  int ToRow = highs_model.lp_.numRow_ - 1;
  int numExtractRows = ToRow - FmRow + 1;
  vector<double> XrowLower;
  vector<double> XrowUpper;
  vector<int> XARstart;
  vector<int> XARindex;
  vector<double> XARvalue;
  //  model.util_extractRows(FmRow, ToRow, &(*XrowLower.begin()),
  //  &(*XrowUpper.begin()),
  // &(*XARstart.begin()), &(*XARindex.begin()), &(*XARvalue.begin()));

  //  printf("Returned from model.util_extractRows with\n");
  //  model.util_reportRowVec(numExtractRows, XrowLower, XrowUpper);
  //  model.util_reportRowMtx(numExtractRows, XARstart, XARindex, XARvalue);

  // Delete the rows just extracted
  model.util_deleteRows(FmRow, ToRow);
  //  model.util_reportModel();

  // Extract all remaining rows
  FmRow = 0;
  ToRow = highs_model.lp_.numRow_ - 1;
  int num0ExtractRows = ToRow - FmRow + 1;
  vector<double> X0rowLower;
  vector<double> X0rowUpper;
  vector<int> X0ARstart;
  vector<int> X0ARindex;
  vector<double> X0ARvalue;

  // model.util_extractRows(FmRow, ToRow, &(*X0rowLower.begin()),
  // &(*X0rowUpper.begin()),
  //			 &(*X0ARstart.begin()), &(*X0ARindex.begin()),
  //&(*X0ARvalue.begin()));

  // Delete the rows just extracted
  model.util_deleteRows(FmRow, ToRow);
  //  model.util_reportModel();

  // Extract all remaining columns
  FmCol = 0;
  ToCol = highs_model.lp_.numCol_ - 1;
  int num0ExtractCols = ToCol - FmCol + 1;
  vector<double> X0colCost;
  vector<double> X0colLower;
  vector<double> X0colUpper;
  vector<int> X0Astart;
  vector<int> X0Aindex;
  vector<double> X0Avalue;
  //  model.util_extractCols(FmCol, ToCol, X0colCost, X0colLower, X0colUpper,
  //			 X0Astart, X0Aindex, X0Avalue);

  // Delete the columns just extracted
  model.util_deleteCols(FmCol, ToCol);
  //  model.util_reportModel();

  int nnonz = 0;
  model.util_addCols(num0ExtractCols, &X0colCost[0], &X0colLower[0],
                     &X0colUpper[0], nnonz, &X0Astart[0], &X0Aindex[0],
                     &X0Avalue[0]);
  //  model.util_reportModel();

  nnonz = X0ARstart[num0ExtractRows];
  model.util_addRows(num0ExtractRows, &X0rowLower[0], &X0rowUpper[0], nnonz,
                     &X0ARstart[0], &X0ARindex[0], &X0ARvalue[0]);
  //  model.util_reportModel();

  nnonz = XARstart[numExtractRows];
  model.util_addRows(numExtractRows, &XrowLower[0], &XrowUpper[0], nnonz,
                     &XARstart[0], &XARindex[0], &XARvalue[0]);
  //  model.util_reportModel();

  nnonz = XAstart[numExtractCols];
  model.util_addCols(numExtractCols, &XcolCost[0], &XcolLower[0], &XcolUpper[0],
                     nnonz, &XAstart[0], &XAindex[0], &XAvalue[0]);
  //  model.util_reportModel();

  simplex_method_.scale_solver_lp(highs_model);
  HDual dual_solver(highs_model);
  dual_solver.solve();
  //  reportLpSolution(highs_model);
  model.util_reportSolverOutcome("SCIP 1");

  vector<double> colPrimal(highs_model.lp_.numCol_);
  vector<double> colDual(highs_model.lp_.numCol_);
  vector<double> colLower(highs_model.lp_.numCol_);
  vector<double> colUpper(highs_model.lp_.numCol_);
  vector<double> rowPrimal(highs_model.lp_.numRow_);
  vector<double> rowDual(highs_model.lp_.numRow_);
  vector<double> rowLower(highs_model.lp_.numRow_);
  vector<double> rowUpper(highs_model.lp_.numRow_);
  model.util_getPrimalDualValues(colPrimal, colDual, rowPrimal, rowDual);
  model.util_getColBounds(highs_model.solver_lp_, 0, highs_model.lp_.numCol_ - 1, &colLower[0], &colUpper[0]);
  model.util_getRowBounds(highs_model.solver_lp_, 0, highs_model.lp_.numRow_ - 1, &rowLower[0], &rowUpper[0]);

  double og_colLower;
  double og_colUpper;
  int colBoundIndex;
  double nw_colLower;
  double nw_colUpper;

  int num_resolve = 0;
  for (int col = 0; col < highs_model.lp_.numCol_; col++) {
    //    model.util_getColBounds(model.solver_lp_, col, col, &og_colLower, &og_colUpper);
    printf("\nColumn %2d has primal value %11g and bounds [%11g, %11g]", col,
           colPrimal[col], og_colLower, og_colUpper);
    if (model.basis_->nonbasicFlag_[col]) {
      printf(": nonbasic so don't branch\n");
      continue;
    } else {
      double rsdu =
          min(colPrimal[col] - og_colLower, og_colUpper - colPrimal[col]);
      if (rsdu < 0.1) {
        printf(": basic but rsdu = %11g so don't branch\n", rsdu);
        continue;
      }
      printf(": basic with rsdu = %11g so branch\n\n", rsdu);
      num_resolve++;
      colBoundIndex = col;
      if (highs_isInfinity(og_colUpper))
        nw_colLower = colPrimal[col] + 1;
      else
        nw_colLower = og_colUpper;
      nw_colUpper = og_colUpper;
      printf("Calling model.util_chgColBounds(1, %d, %g, %g)\n", colBoundIndex,
             nw_colLower, nw_colUpper);
      model.util_chgColBoundsSet(1, &colBoundIndex, &nw_colLower, &nw_colUpper);
      printf("Calling scale_solver_lp(highs_model)\n");
      simplex_method_.scale_solver_lp(highs_model);
      dual_solver.solve();
      model.util_reportSolverOutcome("SCIP 2");
      // Was &nw_colLower, &nw_colUpper); and might be more interesting for
      // avgas
      model.util_chgColBoundsSet(1, &colBoundIndex, &og_colLower, &og_colUpper);
      if (num_resolve >= 10) break;
    }
  }
  printf("Returning from solveSCIP\n");
  cout << flush;
  return HighsStatus::OK;
}

// Single function to solve an lp according to options and fill
// solution in solution.
HighsStatus runSimplexSolver(const HighsOptions& opt,
                             HighsModelObject& highs_model) {
  // For the moment handle scip case separately.
  if (opt.scip) return solveScip(opt, highs_model);

  HighsTimer &timer = highs_model.timer_;

  // When runSimplexSolver is called initialize an instance of HModel inside the
  // HighsModelObject. This will then be passed to HDual.
  highs_model.hmodel_.push_back(HModel());

  HModel& model = highs_model.hmodel_[0];
  const HighsLp &lp_ = highs_model.lp_;

  // Give model the HiGHS Model Object run clock for timeout purposes
  //  model.modelTotalClock = highs_model.modelTotalClock;

  // Set pointers within HModel
  model.basis_ = &highs_model.basis_;
  model.scale_ = &highs_model.scale_;
  model.simplex_info_ = &highs_model.simplex_info_;
  model.solver_lp_ = &highs_model.solver_lp_;
  model.matrix_ = &highs_model.matrix_;
  model.factor_ = &highs_model.factor_;

  HighsSimplexInfo &simplex_info_ = highs_model.simplex_info_;
  // Copy the LP to the structure to be used by the solver
  highs_model.solver_lp_ = highs_model.lp_;

  // Set simplex options from HiGHS options
  HSimplex simplex_method_;
  simplex_method_.options(highs_model, opt);

  // Possibly transpose the LP to be solved. This will change the
  // numbers of rows and columns in the LP to be solved
  if (simplex_info_.transpose_solver_lp) simplex_method_.transpose_solver_lp(highs_model);

  // Now that the numbers of rows and columns in the LP to be solved
  // are fixed, initialise the real and integer random vectors
  simplex_method_.initialiseSolverLpRandomVectors(highs_model);
  //
  // Allocate memory for the basis
  // assignBasis();
  const int numTot = highs_model.lp_.numCol_ + highs_model.lp_.numRow_;
  highs_model.basis_.basicIndex_.resize(highs_model.lp_.numRow_);
  highs_model.basis_.nonbasicFlag_.assign(numTot, 0);
  highs_model.basis_.nonbasicMove_.resize(numTot);
  //
  // Possibly scale the LP to be used by the solver
  //
  // Initialise unit scaling factors, to simplify things is no scaling
  // is performed
  simplex_method_.scaleHighsModelInit(highs_model);
  if (simplex_info_.scale_solver_lp)
    simplex_method_.scale_solver_lp(highs_model);
  //
  // Possibly permute the columns of the LP to be used by the solver. 
  if (simplex_info_.permute_solver_lp)
    simplex_method_.permute_solver_lp(highs_model);
  //
  // Possibly tighten the bounds of LP to be used by the solver. 
  if (simplex_info_.tighten_solver_lp)
    simplex_method_.tighten_solver_lp(highs_model);
  //


  model.initWithLogicalBasis();

  HighsLp &solver_lp_ = highs_model.solver_lp_;
  highs_model.matrix_.setup_lgBs(solver_lp_.numCol_, solver_lp_.numRow_,
				  &solver_lp_.Astart_[0],
				  &solver_lp_.Aindex_[0],
				  &solver_lp_.Avalue_[0]);

  highs_model.factor_.setup(solver_lp_.numCol_, solver_lp_.numRow_,
			   &solver_lp_.Astart_[0],
			   &solver_lp_.Aindex_[0],
			   &solver_lp_.Avalue_[0],
			   &highs_model.basis_.basicIndex_[0]);

  // Set pointers within HModel for the matrix and factor data structure
  //  model.matrix_ = &highs_model.matrix_;
  //  model.factor_ = &highs_model.factor_;


  // Crash, if needed.

  HighsStatus result = solveSimplex(opt, highs_model);

  // todo uncomment line below.
  if (result != HighsStatus::Optimal) return result;

  // HighsSolution set values in highs_model.
  HighsSolution& solution = highs_model.solution_;
  highs_model.hmodel_[0].util_getPrimalDualValues(
      solution.colValue_, solution.colDual_, solution.rowValue_,
      solution.rowDual_);
  model.util_getBasicIndexNonbasicFlag(highs_model.basis_info_.basis_index,
                                       highs_model.basis_info_.nonbasic_flag);

  highs_model.basis_info_.nonbasic_move = model.basis_->nonbasicMove_;

  return result;
}

#endif
