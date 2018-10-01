#ifndef POINCARE_FUNCTION_H
#define POINCARE_FUNCTION_H

#include <poincare/symbol.h>
#include <poincare/variable_context.h>

namespace Poincare {

class FunctionNode : public SymbolNode  {
public:
  // TreeNode
  size_t size() const override { return sizeof(FunctionNode); }
  int numberOfChildren() const override { return 1; } //TODO allow any number of children? Needs templating
#if POINCARE_TREE_LOG
  virtual void logNodeName(std::ostream & stream) const override {
    stream << "Function";
  }
#endif

  // Properties
  Type type() const override { return Type::Function; }
  Expression replaceSymbolWithExpression(const Symbol & symbol, const Expression & expression) override;
  int polynomialDegree(Context & context, const char * symbolName) const override;
  int getPolynomialCoefficients(Context & context, const char * symbolName, Expression coefficients[]) const override;
  int getVariables(Context & context, isVariableTest isVariable, char * variables, int maxSizeVariable) const override;
  float characteristicXRange(Context & context, Preferences::AngleUnit angleUnit) const override;

private:
  VariableContext xContext(Context & parentContext) const;
  // Layout
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  // Simplification
  Expression shallowReduce(Context & context, Preferences::AngleUnit angleUnit, bool replaceSymbols = true) override;
  // Evaluation
  Evaluation<float> approximate(SinglePrecision p, Context& context, Preferences::AngleUnit angleUnit) const override;
  Evaluation<double> approximate(DoublePrecision p, Context& context, Preferences::AngleUnit angleUnit) const override;
};

class Function : public Symbol {
public:
  explicit Function(const char * name);
  Function(const FunctionNode * n) : Symbol(n) {}
  explicit Function(const char * name, Expression operand) : Function(name) {
    replaceChildAtIndexInPlace(0, operand);
  }

  Expression replaceSymbolWithExpression(const Symbol & symbol, const Expression & expression);
  Expression shallowReduce(Context & context, Preferences::AngleUnit angleUnit, bool replaceSymbols = true);
};

}

#endif
