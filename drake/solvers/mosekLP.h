// Copyright 2016. Alex Dunyak
#pragma once


extern "C" {
  #include "build/include/mosek/mosek.h"  // Make sure to figure out how to put in makefile
}

#include <Eigen/Sparse>
#include <Eigen/Core>
#include <vector>
#include <string>

#include "drake/solvers/solution_result.h"
#include "drake/solvers/Optimization.h"
#include "drake/solvers/MathematicalProgram.h"

/*Definitions and program flow taken from
http://docs.mosek.com/7.1/capi/Linear_optimization.html

For further definition of terms, see
http://docs.mosek.com/7.1/capi/Conventions_employed_in_the_API.html
*/
namespace drake {
namespace solvers {

class DRAKEOPTIMIZATION_EXPORT mosekLP :
      public MathematicalProgramSolverInterface {
  /*mosekLP
   *@brief this class allows the creation and solution of a linear programming
   *problem using the mosek solver.
   */
 public:
    mosekLP() {
      task = NULL;
      env = NULL;
      aptrb = (MSKint32t *) malloc(sizeof(MSKint32t));
      aptre = (MSKint32t *) malloc(sizeof(MSKint32t));
      asub = (MSKint32t *) malloc(sizeof(MSKint32t));
      aval = (double *) malloc(sizeof(double));
      bkc = (MSKboundkeye *) malloc(sizeof(MSKboundkeye));
      blc = (double *) malloc(sizeof(double));
      buc = (double *) malloc(sizeof(double));
      bkx = (MSKboundkeye *) malloc(sizeof(MSKboundkeye));
      blx = (double *) malloc(sizeof(double));
      bux = (double *) malloc(sizeof(double));
    }
  /* Create a mosek linear programming environment using a constraint matrix
  * Takes the number of variables and constraints, the linear eqn to
  * optimize, the constraint matrix, and the constraint and variable bounds
  * @p environment is created and must be told to optimize.
  */
  mosekLP(int num_variables, int num_constraints,
      std::vector<double> equationScalars,
      Eigen::MatrixXd cons_,
      std::vector<MSKboundkeye> mosek_constraint_bounds_,
      std::vector<double> upper_constraint_bounds_,
      std::vector<double> lower_constraint_bounds_,
      std::vector<MSKboundkeye> mosek_variable_bounds_,
      std::vector<double> upper_variable_bounds_,
      std::vector<double> lower_variable_bounds_);

  /* Create a mosek linear programming environment using a sparse column
  * constraint matrix
  * Takes the number of variables and constraints, the linear eqn to
  * optimize, the constraint matrix, and the constraint and variable bounds
  * @p environment is created and must be told to optimize.
  */
  mosekLP(int num_variables, int num_constraints,
      std::vector<double> equationScalars,
      Eigen::SparseMatrix<double> sparsecons_,
      std::vector<MSKboundkeye> mosek_constraint_bounds_,
      std::vector<double> upper_constraint_bounds_,
      std::vector<double> lower_constraint_bounds_,
      std::vector<MSKboundkeye> mosek_variable_bounds_,
      std::vector<double> upper_variable_bounds_,
      std::vector<double> lower_variable_bounds_);

  ~mosekLP() {
    if (task != NULL)
      MSK_deletetask(&task);
    if (env != NULL)
      MSK_deleteenv(&env);
    free(aptrb);
    free(aptre);
    free(asub);
    free(aval);
    free(bkc);
    free(blc);
    free(buc);
    free(bkx);
    free(blx);
    free(bux);
  }

    /* Solve()
   * @brief optimizes variables in given linear constraints, works with either
   * of the two previous object declarations.
   * */
  SolutionResult Solve(OptimizationProblem &prog) const override;

  bool available() const override;

  std::vector<double>  GetSolution() const { return solutions_; }

  Eigen::VectorXd GetEigenVectorSolutions() const {
    Eigen::VectorXd soln_(numvar);
    if (solutions_.empty())
      return soln_;
    int i = 0;
    for (i = 0; i < solutions_.size(); ++i) {
      soln_(i) = solutions_[i];
    }
    return soln_.transpose();
  }

 private:
  /*The following names are consistent with the example programs given by
   *http://docs.mosek.com/7.1/capi/Linear_optimization.html
   *and by
   *http://docs.mosek.com/7.1/capi/Conventions_employed_in_the_API.html
   *to ensure ease of translation and understanding.
   */

   /*AddLinearConstraintMatrix()
   *@brief adds linear constraints to mosek environment
   */
  void AddLinearConstraintMatrix(const Eigen::MatrixXd& cons_);

  /*addLinearConstraintSparseColumnMatrix()
  *@brief adds linear constraints in sparse column matrix form
  *just uses Eigen's sparse matrix library to implement expected format
  */
  void AddLinearConstraintSparseColumnMatrix(
    const Eigen::SparseMatrix<double>& sparsecons_);

  /*AddLinearConstraintBounds()
  *@brief bounds constraints, see http://docs.mosek.com/7.1/capi/Conventions_employed_in_the_API.html
  *for details on how to set mosek_bounds_
  */
  void AddLinearConstraintBounds(const std::vector<MSKboundkeye>& mosek_bounds_,
      const std::vector<double>& upper_bounds_,
      const std::vector<double>& lower_bounds_);

  /*AddVariableBounds()
   * @brief bounds variables, see http://docs.mosek.com/7.1/capi/Conventions_employed_in_the_API.html
   * for details on how to set mosek_bounds_
   */
  void AddVariableBounds(const std::vector<MSKboundkeye>& mosek_bounds_,
                         const std::vector<double>& upper_bounds_,
                         const std::vector<double>& lower_bounds_);

  /*FindMosekBounds()
   * Given upper and lower bounds for a variable or constraint, finds the
   * equivalent Mosek bound keys.
   */
  std::vector<MSKboundkeye> FindMosekBounds(
      const std::vector<double>& upper_bounds_,
      const std::vector<double>& lower_bounds_) const;

  SolutionResult OptimizeTask(const std::string& maxormin,
                              const std::string& ptype);

  MSKint32t numvar, numcon;
  MSKint32t* aptrb;   // Where ptrb[j] is the position of the first
                      // value/index in aval / asub for column j.
  MSKint32t*  aptre;  // Where ptre[j] is the position of the last
                      // value/index plus one in aval / asub for column j.
  MSKint32t* asub;    // list of row indices
  double* aval;       // list of nonzero entries of A ordered by columns
  MSKboundkeye* bkc;  // mosek notation bounds on constraints
  double *blc, *buc;  // bounds on constraints
  MSKboundkeye *bkx;  // mosek notation bounds on variables
  double *blx, *bux;  // bounds on variables
  MSKenv_t env;       // Internal environment, used to check if problem formed
                      // correctly
  MSKtask_t task;     // internal definition of task
  MSKrescodee r;      // used for validity checking
  std::vector<double> solutions_;  // Contains the solutions of the system
};

}  // namespace solvers
}  // namespace drake
