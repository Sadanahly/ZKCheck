/*
* Cppcheck - A tool for static C/C++ code analysis
* Copyright (C) 2007-2016 Cppcheck team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//----------------------------------------------------------------------
#ifndef checkFuncParaVoidH  
#define checkFuncParaVoidH
//-----------------------------------------------------------------------

#include "config.h"
#include "check.h"

class Tokenizer;

class ZKCHECKLIB CheckFuncParaVoid : public Check {
public:
	/** This constructor is used when registering the CheckClass */
	CheckFuncParaVoid() : Check(myName()) {
	}

	/** This constructor is used when running checks. */
	CheckFuncParaVoid(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		:Check(myName(), tokenizer, settings, errorLogger){
	}

	/** @brief Run checks against the normal token list */
	/*void runRawChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckFuncParaVoid checkFuncParaVoid(tokenizer, settings, errorLogger);
		checkFuncParaVoid.funcParaVoid();
	}*/

	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckFuncParaVoid checkFuncParaVoid(tokenizer, settings, errorLogger);
		checkFuncParaVoid.funcParaVoid();
	}

	/** @brief Run checks against the normal token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckFuncParaVoid checkFuncParaVoid(tokenizer, settings, errorLogger);
		//checkFuncParaVoid.funcParaVoid();
	}

	/** check there exits void if functions without parameter*/
	void funcParaVoid();

private:

	//const Tokenizer *_tokenizer;

	bool isFunctionDecl(const Token *tok);

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
		CheckFuncParaVoid c(0, settings, errorLogger);
		c.errorFunctionPatameter(0);
	}

	void errorFunctionPatameter(const Token *tok);

	static std::string myName() {
		return "FuncParaVoid";
	}

	std::string classInfo() const {
		return "Warn if there is no void when function with no parameter";
	}
};


#endif