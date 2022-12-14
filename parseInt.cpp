/* Implementation of Recursive-Descent Parser
 * parse.cpp
 * Programming Assignment 2
 * Spring 2021
*/

#include "parserInt.h"
#include <iostream>
using namespace std;

map<string, bool> defVar;
map<string, Token> SymTable;
map<string, Value> TempsResults; //Container of temporary locations of Value objects for results of expressions, variables values and constance 
queue <Value>* ValQue; //declare a pointer variable to a queue of Value objects

namespace Parser {
	bool pushed_back = false;
	LexItem	pushed_token;

	static LexItem GetNextToken(istream& in, int& line) {
		if (pushed_back) {
			pushed_back = false;
			return pushed_token;
		}
		return getNextToken(in, line);
	}

	static void PushBackToken(LexItem& t) {
		if (pushed_back) {
			abort();
		}
		pushed_back = true;
		pushed_token = t;
	}

}

static int error_count = 0;

int ErrCount()
{
	return error_count;
}

void ParseError(int line, string msg)
{
	++error_count;
	cout << line << ": " << msg << endl;
}





//Program is: Prog = PROGRAM IDENT {Decl} {Stmt} END PROGRAM IDENT
bool Prog(istream& in, int& line)
{
	bool dl = false, sl = false;
	LexItem tok = Parser::GetNextToken(in, line);


	if (tok.GetToken() == PROGRAM) {
		tok = Parser::GetNextToken(in, line);
		if (tok.GetToken() == IDENT) {
			dl = Decl(in, line);
			if (!dl)
			{
				ParseError(line, "Incorrect Declaration in Program");
				return false;
			}
			sl = Stmt(in, line);
			if (!sl)
			{
				ParseError(line, "Incorrect Statement in program");
				return false;
			}
			tok = Parser::GetNextToken(in, line);

			if (tok.GetToken() == END) {
				tok = Parser::GetNextToken(in, line);

				if (tok.GetToken() == PROGRAM) {
					tok = Parser::GetNextToken(in, line);

					if (tok.GetToken() == IDENT) {
						return true;
					}
					else
					{
						ParseError(line, "Missing Program Name");
						return false;
					}
				}
				else
				{
					ParseError(line, "Missing PROGRAM at the End");
					return false;
				}
			}
			else
			{
				ParseError(line, "Missing END of Program");
				return false;
			}
		}
	}
	else if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}

	return false;
}

//Decl = Type : VarList 
//Type = INTEGER | REAL | CHAR
bool Decl(istream& in, int& line) {
	bool status = false;
	LexItem tok;

	LexItem t = Parser::GetNextToken(in, line);


	if (t == INTEGER || t == REAL || t == CHAR) {

		tok = Parser::GetNextToken(in, line);
		cout << tok.GetLexeme() << ' ' << tok.GetToken() << endl;
		if (tok.GetToken() == COLON) {
			status = IdList(in, line, t);

			if (status)
			{
				status = Decl(in, line);
				return status;
			}
		}
		else {
			ParseError(line, "Missing Colon");
			return false;
		}
	}

	Parser::PushBackToken(t);
	return true;
}

//Stmt is either a PrintStmt, ReadStmt, IfStmt, or an AssigStmt
//Stmt = AssigStmt | IfStmt | PrintStmt | ReadStmt
bool Stmt(istream& in, int& line) {
	bool status;

	LexItem t = Parser::GetNextToken(in, line);

	switch (t.GetToken()) {

	case PRINT:
		status = PrintStmt(in, line);

		if (status)
			status = Stmt(in, line);
		break;

	case IF:
		status = IfStmt(in, line);
		if (status)
			status = Stmt(in, line);
		break;

	case IDENT:
		Parser::PushBackToken(t);
		status = AssignStmt(in, line);
		if (status)
			status = Stmt(in, line);
		break;

	case READ:
		status = ReadStmt(in, line);

		if (status)
			status = Stmt(in, line);
		break;

	default:
		Parser::PushBackToken(t);
		return true;
	}

	return status;
}
bool ReadStmt(istream& in, int& line)
{

	LexItem t;

	if ((t = Parser::GetNextToken(in, line)) != COMA) {

		ParseError(line, "Missing a Comma");
		return false;
	}

	bool ex = VarList(in, line);

	if (!ex) {
		ParseError(line, "Missing Variable after Read Statement");
		return false;
	}



	return ex;
}
//PrintStmt:= print, ExpreList 
bool PrintStmt(istream& in, int& line) {
	LexItem t;
	if ((t = Parser::GetNextToken(in, line)) != COMA) {
		ParseError(line, "Missing a Comma");
		return false;
	}
	bool ex = ExprList(in, line);
	if (!ex) {
		ParseError(line, "Missing expression after print");
		while (!(*ValQue).empty()) {
			ValQue->pop();
		}
		delete ValQue;
		return false;
	}
	while (!(*ValQue).empty()) {
		Value nextVal = (*ValQue).front();
		cout << nextVal;
		ValQue->pop();
	}
	cout << endl;
	return ex;
}

//IfStmt:= if (Expr) then {Stmt} END IF
bool IfStmt(istream& in, int& line) {
	bool ex = false;
	LexItem t;
	Value retVal;
	if ((t = Parser::GetNextToken(in, line)) != LPAREN) {

		ParseError(line, "Missing Left Parenthesis");
		return false;
	}

	ex = LogicExpr(in, line, retVal);
	if (!ex) {
		ParseError(line, "Missing if statement Logic Expression");
		return false;
	}

	if ((t = Parser::GetNextToken(in, line)) != RPAREN) {

		ParseError(line, "Missing Right Parenthesis");
		return false;
	}

	if ((t = Parser::GetNextToken(in, line)) != THEN) {

		ParseError(line, "Missing THEN");
		return false;
	}
	if (retVal.GetBool())
	{
		bool st = Stmt(in, line);
		if (!st) {
			ParseError(line, "Missing statement for IF");
			return false;
		}



		if ((t = Parser::GetNextToken(in, line)) != END) {

			ParseError(line, "Missing END of IF");
			return false;
		}
		if ((t = Parser::GetNextToken(in, line)) != IF) {

			ParseError(line, "Missing IF at End of IF statement");
			return false;
		}
	}
	return true;
}


//IdList:= IDENT {,IDENT}
bool IdList(istream& in, int& line, LexItem& tok) {
	bool status = false;
	string identstr;

	LexItem t = Parser::GetNextToken(in, line);
	if (t == IDENT)
	{
		//set IDENT lexeme to the type tok value
		identstr = t.GetLexeme();
		if (!(defVar.find(identstr)->second))
		{
			defVar[identstr] = true;
			SymTable[identstr] = tok.GetToken();
		}
		else
		{
			ParseError(line, "Variable Redefinition");
			return false;
		}

	}
	else
	{
		ParseError(line, "Missing Variable");
		return false;
	}

	t = Parser::GetNextToken(in, line);

	if (t == COMA) {
		status = IdList(in, line, tok);
	}
	else if (t.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << t.GetLexeme() << ")" << endl;
		return false;
	}
	else {
		Parser::PushBackToken(tok);
		return true;
	}
	return status;
}

//VarList
bool VarList(istream& in, int& line)
{
	bool status = false;
	string identstr;
	LexItem tok = Parser::GetNextToken(in, line);
	status = Var(in, line, tok);

	if (!status)
	{
		ParseError(line, "Missing Variable");
		return false;
	}

	tok = Parser::GetNextToken(in, line);

	if (tok == COMA) {

		status = VarList(in, line);
	}
	else if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}
	else {
		Parser::PushBackToken(tok);
		return true;
	}
	return status;
}

//Var:= ident
bool Var(istream& in, int& line, LexItem& tok)
{
	//called only from the AssignStmt function
	string identstr;



	if (tok == IDENT) {
		identstr = tok.GetLexeme();
		if (!(defVar.find(identstr)->second))
		{
			ParseError(line, "Undeclared Variable");
			return false;

		}
		return true;
	}
	else if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}
	return false;
}

//AssignStmt:= Var = Expr
bool AssignStmt(istream& in, int& line) {

	bool varstatus = false, status = false;
	LexItem t;
	LexItem tok = Parser::GetNextToken(in, line);
	varstatus = Var(in, line, tok);


	if (varstatus) {
		t = Parser::GetNextToken(in, line);

		if (t == ASSOP) {
			Value val;
			status = Expr(in, line, val);
			if (!status) {
				ParseError(line, "Missing Expression in Assignment Statment");
				return status;
			}
			//if((SymTable.find(tok.GetLexeme())->second)==val.GetType())
			TempsResults[tok.GetLexeme()] = val;
			/*else
				throw "RUNTIME ERROR: Can't assign";*/

		}
		else if (t.GetToken() == ERR) {
			ParseError(line, "Unrecognized Input Pattern");
			cout << "(" << t.GetLexeme() << ")" << endl;
			return false;
		}
		else {
			ParseError(line, "Missing Assignment Operator =");
			return false;
		}
	}
	else {
		ParseError(line, "Missing Left-Hand Side Variable in Assignment statement");
		return false;
	}
	return status;
}

//ExprList:= Expr {,Expr}
bool ExprList(istream& in, int& line) {
	bool status = false;
	Value retVal;
	status = Expr(in, line, retVal);
	ValQue->push(retVal);
	if (!status) {
		ParseError(line, "Missing Expression");
		return false;
	}

	LexItem tok = Parser::GetNextToken(in, line);

	if (tok == COMA) {
		status = ExprList(in, line);
	}
	else if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}
	else {
		Parser::PushBackToken(tok);
		return true;
	}
	return status;
}

//Expr:= Term {(+|-) Term}
bool Expr(istream& in, int& line, Value& retVal) {
	Value val1, val2;
	bool t1 = Term(in, line, val1);
	LexItem tok;

	if (!t1) {
		return false;
	}

	tok = Parser::GetNextToken(in, line);
	if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}
	while (tok == PLUS || tok == MINUS)
	{
		t1 = Term(in, line, val2);
		if (!t1)
		{
			ParseError(line, "Missing operand after operator");
			return false;
		}
		if (retVal.GetType() == VCHAR || val2.GetType() == VCHAR) {
			ParseError(line, "Run-Time Error-Illegal Mixed Type Operands");
			return false;
		}
		else {
			if (tok == PLUS) { retVal = retVal + val2; }
			else if (tok == MINUS) { retVal = retVal - val2; }
		}
		tok = Parser::GetNextToken(in, line);
		if (tok.GetToken() == ERR) {
			ParseError(line, "Unrecognized Input Pattern");
			cout << "(" << tok.GetLexeme() << ")" << endl;
			return false;
		}


	}
	Parser::PushBackToken(tok);
	return true;
}

//Term:= SFactor {(*|/) SFactor}
bool Term(istream& in, int& line, Value& retVal) {
	Value val1, val2;
	bool t1 = SFactor(in, line, val1);
	retVal = val1;
	LexItem tok;

	if (!t1) {
		return false;
	}

	tok = Parser::GetNextToken(in, line);
	if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}
	while (tok == MULT || tok == DIV)
	{
		t1 = SFactor(in, line, val2);

		if (!t1) {
			ParseError(line, "Missing operand after operator");
			return false;
		}
		if (retVal.GetType() == VCHAR || val2.GetType() == VCHAR) {
			ParseError(line, "Run-Time Error-Illegal Mixed Type Operands");
			return false;
		}
		else {
			if (tok == MULT) { retVal = retVal * val2; }
			else if (tok == DIV) { retVal = retVal / val2; }
		}
		tok = Parser::GetNextToken(in, line);
		if (tok.GetToken() == ERR) {
			ParseError(line, "Unrecognized Input Pattern");
			cout << "(" << tok.GetLexeme() << ")" << endl;
			return false;
		}

	}
	Parser::PushBackToken(tok);
	return true;
}

//SFactor = Sign Factor | Factor
bool SFactor(istream& in, int& line, Value& retVal)
{
	LexItem t = Parser::GetNextToken(in, line);
	bool status;
	int sign = 0;
	if (t == MINUS)
	{
		sign = -1;
	}
	else if (t == PLUS)
	{
		sign = 1;
	}
	else
		Parser::PushBackToken(t);

	status = Factor(in, line, sign, retVal);
	return status;
}
//LogicExpr = Expr (== | <) Expr
bool LogicExpr(istream& in, int& line, Value& retVal)
{
	Value val1, val2;
	bool t1 = Expr(in, line, val1);
	LexItem tok;

	if (!t1) {
		return false;
	}

	tok = Parser::GetNextToken(in, line);
	if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}
	if (tok == LTHAN || tok == EQUAL)
	{
		t1 = Expr(in, line, val2);
		if (!t1)
		{
			ParseError(line, "Missing expression after relational operator");
			return false;
		}
		if (tok == LTHAN)
		{
			if ((val1 < val2).GetBool())
				retVal = new Value(true);
			else
				retVal = new Value(false);

		}
		if (tok == EQUAL)
		{
			if ((val1 == val2).GetBool())
				retVal = new Value(true);
			else
				retVal = new Value(false);

		}
		return true;
	}
	Parser::PushBackToken(tok);
	return true;
}

//Factor := ident | iconst | rconst | sconst | (Expr)
bool Factor(istream& in, int& line, int sign, Value& retVal) {

	LexItem tok = Parser::GetNextToken(in, line);


	if (tok == IDENT) {
		//check if the var is defined 
		string lexeme = tok.GetLexeme();
		if (!(defVar.find(lexeme)->second))
		{
			ParseError(line, "Undefined Variable");
			return false;
		}
		/*if (!(TempsResults.find(lexeme)))
		{
			throw "RUNTIME ERROR: Is not assigned";
			return false;
		}*/
		else
		{
			retVal = new Value(TempsResults.find(lexeme)->second);
		}
		return true;
	}
	else if (tok == ICONST) {
		retVal = new Value(tok.GetLexeme());
		return true;
	}
	else if (tok == SCONST) {
		retVal = new Value(tok.GetLexeme());
		return true;
	}
	else if (tok == RCONST) {
		retVal = new Value(tok.GetLexeme());
		return true;
	}
	else if (tok == LPAREN) {
		bool ex = Expr(in, line, retVal);
		if (!ex) {
			ParseError(line, "Missing expression after (");
			return false;
		}
		if (Parser::GetNextToken(in, line) == RPAREN)
			return ex;

		ParseError(line, "Missing ) after expression");
		return false;
	}
	else if (tok.GetToken() == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}

	ParseError(line, "Unrecognized input");
	return 0;
}
/*Value  Value :: operator+(const Value& op) const
{
	Value ans;
	if (T == op.T)
	{
		if (T == VINT)
			ans = new Value(Itemp + op.Itemp);
		if (T == VREAL)
			ans = new Value(Rtemp + op.Rtemp);
		if (T == VCHAR)
			ans = new Value(Stemp + op.Stemp);
		if (T == VBOOL)
			ans = new Value(Btemp + op.Btemp);
	}
	else if (T == VINT && op.T == VREAL)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Itemp + op.Rtemp);
	}
	else if (T == VREAL && op.T == VINT)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Rtemp + op.Itemp);

	}
	else
	{
		Value ans();
		throw "RUNTIME ERROR: Types do not match";
	}
	return ans;
}
// subtract op from this
Value Value :: operator-(const Value& op) const
{
	Value ans;
	if (T == op.T)
	{
		if (T == VINT)
			ans = new Value(Itemp - op.Itemp);
		if (T == VREAL)
			ans = new Value(Rtemp - op.Rtemp);
	}
	else if (T == VINT && op.T == VREAL)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Itemp - op.Rtemp);
	}
	else if (T == VREAL && op.T == VINT)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Rtemp - op.Itemp);

	}
	else
	{
		Value ans();
		throw "RUNTIME ERROR: Types do not match";
	}
	return ans;
}
// multiply this by op
Value Value :: operator*(const Value& op) const
{
	Value ans;
	if (T == op.T)
	{
		if (T == VINT)
			ans = new Value(Itemp * op.Itemp);
		if (T == VREAL)
			ans = new Value(Rtemp * op.Rtemp);

	}
	else if (T == VINT && op.T == VREAL)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Itemp * op.Rtemp);
	}
	else if (T == VREAL && op.T == VINT)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Rtemp * op.Itemp);

	}
	else
	{
		Value ans();
		throw "RUNTIME ERROR: Types do not match";
	}
	return ans;
}
// divide this by op
Value Value :: operator/(const Value& op) const
{
	Value ans;
	if (T == op.T)
	{
		if (T == VINT)
			ans = new Value(Itemp / op.Itemp);
		if (T == VREAL)
			ans = new Value(Rtemp / op.Rtemp);

	}
	else if (T == VINT && op.T == VREAL)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Itemp / op.Rtemp);
	}
	else if (T == VREAL && op.T == VINT)
	{
		ans = new Value();
		ans.SetType(VREAL);
		ans.SetInt(0);
		ans.SetReal(Rtemp / op.Itemp);

	}
	else
	{
		Value ans();
		throw "RUNTIME ERROR: Types do not match";
	}
	return ans;
}
Value Value :: operator==(const Value& op) const
{
	Value ans;
	if (T == op.T)
	{
		if (T == VINT && Itemp==op.Itemp)
			ans = new Value(true);
		else if (T == VREAL && Rtemp == op.Rtemp)
			ans = new Value(true);
		else if (T == VCHAR && Stemp == op.Stemp)
			ans = new Value(true);
		else if (T == VBOOL && Btemp == op.Btemp)
			ans = new Value(true);
		else
			ans = new Value(false);
	}
	else if (T == VINT && op.T == VREAL)
	{
		if (Itemp == op.Rtemp)
			ans = new Value(true);
		else
			ans = new Value(false);
	}
	else if (T == VREAL && op.T == VINT)
	{
		if (Rtemp == op.Itemp)
			ans = new Value(true);
		else
			ans = new Value(false);

	}
	else
	{
		Value ans();
		throw "RUNTIME ERROR: Types do not match";
	}
	return ans;
}

Value Value :: operator<(const Value& op) const
{
	Value ans;
	if (T == op.T)
	{
		if (T == VINT && Itemp < op.Itemp)
			ans = new Value(true);
		else if (T == VREAL && Rtemp < op.Rtemp)
			ans = new Value(true);
		else if (T == VCHAR && Stemp < op.Stemp)
			ans = new Value(true);
		else if (T == VBOOL && Btemp < op.Btemp)
			ans = new Value(true);
		else
			ans = new Value(false);
	}
	else if (T == VINT && op.T == VREAL)
	{
		if (Itemp < op.Rtemp)
			ans = new Value(true);
		else
			ans = new Value(false);
	}
	else if (T == VREAL && op.T == VINT)
	{
		if (Rtemp < op.Itemp)
			ans = new Value(true);
		else
			ans = new Value(false);

	}
	else
	{
		Value ans();
		throw "RUNTIME ERROR: Types do not match";
	}
	return ans;
}
*/

