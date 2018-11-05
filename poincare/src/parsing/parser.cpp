#include "parser.h"

namespace Poincare {

Expression Parser::parse() {
  Expression result = parseUntil(Token::EndOfStream);
  if (m_status == Status::Progress) {
    m_status = Status::Success;
    return result;
  }
  return Expression();
}

Expression Parser::parseUntil(Token::Type stoppingType) {
  typedef void (Parser::*TokenParser)(Expression & leftHandSide);
  static constexpr TokenParser tokenParsers[] = {
    &Parser::parseUnexpected,      // Token::EndOfStream
    &Parser::parseEqual,           // Token::Equal
    &Parser::parseStore,           // Token::Store
    &Parser::parseUnexpected,      // Token::RightBracket
    &Parser::parseUnexpected,      // Token::RightParenthesis
    &Parser::parseUnexpected,      // Token::RightBrace
    &Parser::parseUnexpected,      // Token::Comma
    &Parser::parsePlus,            // Token::Plus
    &Parser::parseMinus,           // Token::Minus
    &Parser::parseTimes,           // Token::Times
    &Parser::parseSlash,           // Token::Slash
    &Parser::parseImplicitTimes,   // Token::ImplicitTimes
    &Parser::parseCaret,           // Token::Power
    &Parser::parseBang,            // Token::Bang
    &Parser::parseMatrix,          // Token::LeftBracket
    &Parser::parseLeftParenthesis, // Token::LeftParenthesis
    &Parser::parseUnexpected,      // Token::LeftBrace
    &Parser::parseEmpty,           // Token::Empty
    &Parser::parseConstant,        // Token::Constant
    &Parser::parseNumber,          // Token::Number
    &Parser::parseIdentifier,      // Token::Identifier
    &Parser::parseUnexpected       // Token::Undefined
  };
  Expression leftHandSide;
  do {
    popToken();
    (this->*(tokenParsers[m_currentToken.type()]))(leftHandSide);
  } while (m_status == Status::Progress && nextTokenHasPrecedenceOver(stoppingType));
  return leftHandSide;
}

void Parser::popToken() {
  if (m_pendingImplicitMultiplication) {
    m_currentToken = Token(Token::ImplicitTimes);
    m_pendingImplicitMultiplication = false;
  } else {
    m_currentToken = m_nextToken;
    m_nextToken = m_tokenizer.popToken();
  }
}

bool Parser::popTokenIfType(Token::Type type) {
  /* The method called with the Token::Types
   * (Left and Right) Braces, Bracket, Parenthesis and Comma.
   * Never with Token::ImplicitTimes.
   * If this assumption is not satisfied anymore, change the following to handle ImplicitTimes. */
  assert(type != Token::ImplicitTimes && !m_pendingImplicitMultiplication);
  bool tokenTypesCoincide = m_nextToken.is(type);
  if (tokenTypesCoincide) {
    popToken();
  }
  return tokenTypesCoincide;
}

bool Parser::nextTokenHasPrecedenceOver(Token::Type stoppingType) {
  return ((m_pendingImplicitMultiplication) ? Token::ImplicitTimes : m_nextToken.type()) > stoppingType;
}

void Parser::isThereImplicitMultiplication() {
  /* This function is called at the end of
   * parseNumber, parseIdentifier, parseFactorial, parseMatrix, parseLeftParenthesis
   * in order to check whether it should be followed by a Token::ImplicitTimes.
   * In that case, m_pendingImplicitMultiplication is set to true,
   * so that popToken, popTokenIfType, nextTokenHasPrecedenceOver can handle implicit multiplication. */
  m_pendingImplicitMultiplication = (
    m_nextToken.is(Token::Number) ||
    m_nextToken.is(Token::Constant) ||
    m_nextToken.is(Token::Identifier) ||
    m_nextToken.is(Token::LeftParenthesis) ||
    m_nextToken.is(Token::LeftBracket)
  );
}

void Parser::parseUnexpected(Expression & leftHandSide) {
  m_status = Status::Error; // Unexpected Token
}

void Parser::parseNumber(Expression & leftHandSide) {
  if (!leftHandSide.isUninitialized()) {
    m_status = Status::Error; //FIXME
    return;
  }
  leftHandSide = m_currentToken.expression();
  if (m_nextToken.is(Token::Number)) {
    m_status = Status::Error; // No implicite multiplication between two numbers
    return;
  }
  isThereImplicitMultiplication();
}

void Parser::parsePlus(Expression & leftHandSide) {
  Expression rightHandSide;
  if (parseBinaryOperator(leftHandSide, rightHandSide, Token::Plus)) {
    leftHandSide = Addition(leftHandSide, rightHandSide);
  }
}

void Parser::parseEmpty(Expression & leftHandSide) {
  if (!leftHandSide.isUninitialized()) {
    m_status = Status::Error; //FIXME
    return;
  }
  leftHandSide = EmptyExpression();
}

void Parser::parseMinus(Expression & leftHandSide) {
  if (leftHandSide.isUninitialized()) {
    Expression rightHandSide = parseUntil(Token::Slash);
    if (m_status != Status::Progress) {
      return;
    }
    leftHandSide = Opposite(rightHandSide);
  } else {
    Expression rightHandSide = parseUntil(Token::Minus); // Subtraction is left-associative
    if (m_status != Status::Progress) {
      return;
    }
    leftHandSide = Subtraction(leftHandSide, rightHandSide);
  }
}

void Parser::parseTimes(Expression & leftHandSide) {
  Expression rightHandSide;
  if (parseBinaryOperator(leftHandSide, rightHandSide, Token::Times)) {
    leftHandSide = Multiplication(leftHandSide, rightHandSide);
  }
}

void Parser::parseSlash(Expression & leftHandSide) {
  Expression rightHandSide;
  if (parseBinaryOperator(leftHandSide, rightHandSide, Token::Slash)) {
    leftHandSide = Division(leftHandSide, rightHandSide);
  }
}

void Parser::parseImplicitTimes(Expression & leftHandSide) {
  Expression rightHandSide;
  if (parseBinaryOperator(leftHandSide, rightHandSide, Token::Slash)) {
    leftHandSide = Multiplication(leftHandSide, rightHandSide);
  }
}

void Parser::parseCaret(Expression & leftHandSide) {
  Expression rightHandSide;
  if (parseBinaryOperator(leftHandSide, rightHandSide, Token::ImplicitTimes)) {
    leftHandSide = Power(leftHandSide, rightHandSide);
  }
}

void Parser::parseEqual(Expression & leftHandSide) {
  if (leftHandSide.type() == ExpressionNode::Type::Equal) {
    m_status = Status::Error; // Equal is not associative
    return;
  }
  Expression rightHandSide;
  if (parseBinaryOperator(leftHandSide, rightHandSide, Token::Equal)) {
    leftHandSide = Equal(leftHandSide, rightHandSide);
  }
}

void Parser::parseStore(Expression & leftHandSide) {
  if (leftHandSide.isUninitialized()) {
    m_status = Status::Error; // Left-hand side missing.
    return;
  }
  // At this point, m_currentToken is Token::Store.
  popToken();
  const Expression::FunctionHelper * const * functionHelper;
  if (!m_currentToken.is(Token::Identifier) || isReservedFunction(functionHelper) || isSpecialIdentifier()) {
    m_status = Status::Error; // The right-hand side of Token::Store must be symbol or function that is not reserved.
    return;
  }
  Expression rightHandSide;
  parseCustomIdentifier(rightHandSide, m_currentToken.text(), m_currentToken.length());
  if (m_status != Status::Progress) {
    return;
  }
  if (!m_nextToken.is(Token::EndOfStream) ||
      !( rightHandSide.type() == ExpressionNode::Type::Symbol ||
        (rightHandSide.type() == ExpressionNode::Type::Function && rightHandSide.childAtIndex(0).type() == ExpressionNode::Type::Symbol)))
  {
    m_status = Status::Error; // Store expects a single symbol or function.
    return;
  }
  leftHandSide = Store(leftHandSide, static_cast<SymbolAbstract&>(rightHandSide));
}

bool Parser::parseBinaryOperator(const Expression & leftHandSide, Expression & rightHandSide, Token::Type stoppingType) {
  if (leftHandSide.isUninitialized()) {
    m_status = Status::Error; // Left-hand side missing.
    return false;
  }
  rightHandSide = parseUntil(stoppingType);
  if (m_status != Status::Progress) {
    return false;
  }
  if (rightHandSide.isUninitialized()) {
    m_status = Status::Error; //FIXME
    return false;
  }
  return true;
}

void Parser::parseLeftParenthesis(Expression & leftHandSide) {
  if (!leftHandSide.isUninitialized()) {
    m_status = Status::Error; //FIXME
    return;
  }
  leftHandSide = parseUntil(Token::RightParenthesis);
  if (m_status != Status::Progress) {
    return;
  }
  if (!popTokenIfType(Token::RightParenthesis)) {
    m_status = Status::Error; // Right parenthesis missing.
    return;
  }
  leftHandSide = Parenthesis(leftHandSide);
  isThereImplicitMultiplication();
}

void Parser::parseBang(Expression & leftHandSide) {
  if (leftHandSide.isUninitialized()) {
    m_status = Status::Error; // Left-hand side missing
  } else {
    leftHandSide = Factorial(leftHandSide);
  }
  isThereImplicitMultiplication();
}

constexpr const Expression::FunctionHelper * Parser::s_reservedFunctions[];

bool Parser::isReservedFunction(const Expression::FunctionHelper * const * & functionHelper) const {
  // Look up in the array of reserved functions (e.g. Cosine...) whether the current Token corresponds to a reserved function.
  functionHelper = &s_reservedFunctions[0]; // Initialize functionHelper to point at the beginning of the array.
  while (functionHelper < s_reservedFunctionsUpperBound && m_currentToken.compareTo((**functionHelper).name()) > 0) {
    functionHelper++;
  }
  return (functionHelper < s_reservedFunctionsUpperBound && m_currentToken.compareTo((**functionHelper).name()) == 0);
}

bool Parser::isSpecialIdentifier() const {
  // TODO Avoid special cases if possible
  return (
    m_currentToken.compareTo("inf")   == 0 ||
    m_currentToken.compareTo("undef") == 0 ||
    m_currentToken.compareTo("u_")    == 0 ||
    m_currentToken.compareTo("v_")    == 0 ||
    m_currentToken.compareTo("u")     == 0 ||
    m_currentToken.compareTo("v")     == 0 ||
    m_currentToken.compareTo("log_")  == 0
  );
}

void Parser::parseConstant(Expression & leftHandSide) {
  leftHandSide = Constant(m_currentToken.text()[0]);
  isThereImplicitMultiplication();
}

void Parser::parseReservedFunction(Expression & leftHandSide, const Expression::FunctionHelper * const * functionHelper) {
  const char * name = (**functionHelper).name();
  Expression parameters = parseFunctionParameters();
  if (m_status != Status::Progress) {
    return;
  }
  int numberOfParameters = parameters.numberOfChildren();
  while (numberOfParameters > (**functionHelper).numberOfChildren()) {
    functionHelper++;
    if (!(functionHelper < s_reservedFunctionsUpperBound && strcmp(name, (**functionHelper).name()) == 0)) {
      m_status = Status::Error; // Too many parameters provided.
      return;
    }
  }
  if (numberOfParameters < (**functionHelper).numberOfChildren()) {
    m_status = Status::Error; // Too few parameters provided.
    return;
  }
  leftHandSide = (**functionHelper).build(parameters);
  if (leftHandSide.isUninitialized()) {
    m_status = Status::Error; // Incorrect parameter type.
    return;
  }
}

void Parser::parseSequence(Expression & leftHandSide, const char name, Token::Type leftDelimiter, Token::Type rightDelimiter) {
  if (!popTokenIfType(leftDelimiter)) {
    m_status = Status::Error; // Left delimiter missing.
  } else {
    Expression rank = parseUntil(rightDelimiter);
    if (m_status != Status::Progress) {
    } else if (!popTokenIfType(rightDelimiter)) {
      m_status = Status::Error; // Right delimiter missing.
    } else if (rank.isIdenticalTo(Symbol("n",1))) {
      char sym[4] = {name, '(', 'n', ')'};
      leftHandSide = Symbol(sym, 4);
    } else if (rank.isIdenticalTo(Addition(Symbol("n",1),Rational("1")))) {
      char sym[6] = {name, '(', 'n', '+', '1', ')'};
      leftHandSide = Symbol(sym, 6);
    } else {
      m_status = Status::Error; // Unexpected parameter.
    }
  }
}

void Parser::parseSpecialIdentifier(Expression & leftHandSide) {
  if (m_currentToken.compareTo("inf") == 0) {
    leftHandSide = Infinity(false);
  } else if (m_currentToken.compareTo("undef") == 0) {
    leftHandSide = Undefined();
  } else if (m_currentToken.compareTo("u_") == 0 || m_currentToken.compareTo("v_") == 0) { // Special case for sequences (e.g. "u_{n}")
    parseSequence(leftHandSide, m_currentToken.text()[0], Token::LeftBrace, Token::RightBrace);
  } else if (m_currentToken.compareTo("u") == 0 || m_currentToken.compareTo("v") == 0) { // Special case for sequences (e.g. "u(n)")
    parseSequence(leftHandSide, m_currentToken.text()[0], Token::LeftParenthesis, Token::RightParenthesis);
  } else if (m_currentToken.compareTo("log_") == 0) { // Special case for the log function (e.g. "log_{2}(8)")
    if (!popTokenIfType(Token::LeftBrace)) {
      m_status = Status::Error; // Left brace missing.
    } else {
      Expression base = parseUntil(Token::RightBrace);
      if (m_status != Status::Progress) {
      } else if (!popTokenIfType(Token::RightBrace)) {
        m_status = Status::Error; // Right brace missing.
      } else {
        Expression parameter = parseFunctionParameters();
        if (m_status != Status::Progress) {
        } else if (parameter.numberOfChildren() != 1) {
          m_status = Status::Error; // Unexpected number of many parameters.
        } else {
          leftHandSide = Logarithm::Builder(parameter.childAtIndex(0), base);
        }
      }
    }
  }
}

void Parser::parseCustomIdentifier(Expression & leftHandSide, const char * name, size_t length) {
  if (length >= SymbolAbstract::k_maxNameSize) {
    m_status = Status::Error; // Identifier name too long.
    return;
  }
  if (!popTokenIfType(Token::LeftParenthesis)) {
    leftHandSide = Symbol(name, length);
    return;
  }
  Expression parameter = parseCommaSeparatedList();
  if (parameter.numberOfChildren() != 1) {
    m_status = Status::Error; // Unexpected number of paramters.
    return;
  }
  parameter = parameter.childAtIndex(0);
  if (parameter.type() == ExpressionNode::Type::Symbol && strncmp(static_cast<SymbolAbstract&>(parameter).name(),name, length) == 0) {
    m_status = Status::Error; // Function and variable must have distinct names.
  } else if (!popTokenIfType(Token::RightParenthesis)) {
    m_status = Status::Error; // Right parenthesis missing.
  } else {
    leftHandSide = Function(name, length, parameter);
  }
}

void Parser::parseIdentifier(Expression & leftHandSide) {
  if (!leftHandSide.isUninitialized()) {
    m_status = Status::Error; //FIXME
    return;
  }
  const Expression::FunctionHelper * const * functionHelper;
    // If m_currentToken corresponds to a reserved function, the method isReservedFunction
    // will make functionHelper point to an element of s_reservedFunctions.
  if (isReservedFunction(functionHelper)) {
    parseReservedFunction(leftHandSide, functionHelper);
  } else if (isSpecialIdentifier()) {
    parseSpecialIdentifier(leftHandSide);
  } else {
    parseCustomIdentifier(leftHandSide, m_currentToken.text(), m_currentToken.length());
  }
  isThereImplicitMultiplication();
}

Expression Parser::parseFunctionParameters() {
  if (!popTokenIfType(Token::LeftParenthesis)) {
    m_status = Status::Error; // Left parenthesis missing.
    return Matrix();
  }
  if (popTokenIfType(Token::RightParenthesis)) {
    return Matrix(); // The function has no parameter.
  }
  Expression commaSeparatedList = parseCommaSeparatedList();
  if (m_status != Status::Progress) {
    return Matrix();
  }
  if (!popTokenIfType(Token::RightParenthesis)) {
    m_status = Status::Error; // Right parenthesis missing.
    return Matrix();
  }
  return commaSeparatedList;
}

void Parser::parseMatrix(Expression & leftHandSide) {
  if (!leftHandSide.isUninitialized()) {
    m_status = Status::Error; //FIXME
    return;
  }
  Matrix matrix;
  int numberOfRows = 0;
  int numberOfColumns = 0;
  while (!popTokenIfType(Token::RightBracket)) {
    Expression row = parseVector();
    if (m_status != Status::Progress) {
      return;
    }
    if ((numberOfRows == 0 && (numberOfColumns = row.numberOfChildren()) == 0)
        ||
        (numberOfColumns != row.numberOfChildren())) {
      m_status = Status::Error; // Incorrect matrix.
      return;
    } else {
      matrix.addChildrenAsRowInPlace(row, numberOfRows++);
    }
  }
  if (numberOfRows == 0) {
    m_status = Status::Error; // Empty matrix
  } else {
    leftHandSide = matrix;
  }
  isThereImplicitMultiplication();
}

Expression Parser::parseVector() {
  if (!popTokenIfType(Token::LeftBracket)) {
    m_status = Status::Error; // Left bracket missing.
    return Matrix();
  }
  Expression commaSeparatedList = parseCommaSeparatedList();
  if (!popTokenIfType(Token::RightBracket) || m_status != Status::Progress) {
    m_status = Status::Error; // Right bracket missing.
    return Matrix();
  }
  return commaSeparatedList;
}

Expression Parser::parseCommaSeparatedList() {
  Matrix commaSeparatedList;
  int length = 0;
  do {
    Expression item = parseUntil(Token::Comma);
    if (m_status != Status::Progress) {
      return Matrix();
    }
    commaSeparatedList.addChildAtIndexInPlace(item, length, length);
    length++;
  } while (popTokenIfType(Token::Comma));
  return commaSeparatedList;
}

}
