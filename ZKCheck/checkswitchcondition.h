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
#ifndef checkSwitchConditionH  
#define checkSwitchConditionH
//-----------------------------------------------------------------------

#include "config.h"
#include "check.h"

class ZKCHECKLIB CheckSwitchCondition : public Check {
public:
	/** This constructor is used when registering the CheckClass */
	CheckSwitchCondition(): Check(myName()) {
	}

	/** This constructor is used when running checks. */
	CheckSwitchCondition(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		:Check(myName(), tokenizer, settings, errorLogger) {

	} 

	/** @brief Run checks against the normal token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckSwitchCondition checkSwitchCondition(tokenizer, settings, errorLogger);
		checkSwitchCondition.switchCondition();
		checkSwitchCondition.caseDefaultCondition();
	}

	/** check switch...default and case...break*/
	void switchCondition();

	void caseDefaultCondition();
	bool isExistBreak(const Token *start, const Token *end);
	bool isCircleStatement(const Token **tok);


private:
	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
		CheckSwitchCondition c(0, settings, errorLogger);
		c.errorSwitchCondition(0);
		c.errorCaseBreakFault(0);
		
	}

	void errorSwitchCondition(const Token *tok);
	void errorCaseBreakFault(const Token *tok);

	static std::string myName() {
		return "SwitchCondition";
	}

	std::string classInfo() const {
		return "Warn when case...break or switch...default are not matched";
	}
};

#endif
