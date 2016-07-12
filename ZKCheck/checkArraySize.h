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
#ifndef checkArraySizeH  
#define checkArraySizeH
//-----------------------------------------------------------------------

#include "config.h"
#include "check.h"

class ZKCHECKLIB CheckArraySize : public Check {
public:
	/** This constructor is used when registering the CheckClass */
	CheckArraySize() : Check(myName()) {
	}

	/** This constructor is used when running checks. */
	CheckArraySize(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		:Check(myName(), tokenizer, settings, errorLogger) {
	}

	void runRawChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckArraySize checkArraySizes(tokenizer, settings, errorLogger);
		checkArraySizes.allocArraySize();
	}

	/** @brief Run checks against the normal token list */
	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
		CheckArraySize checkArraySizes(tokenizer, settings, errorLogger);
		//checkArraySizes.allocArraySize();
	}

	/** check array size is valid*/
	void allocArraySize();

private:

	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
		CheckArraySize c(0, settings, errorLogger);
		c.errorArraySizeAllocated(0);
	}

	void errorArraySizeAllocated(const Token *tok);

	static std::string myName() {
		return "ArraySize";
	}

	std::string classInfo() const {
		return "Warn if the size of char array is not standard";
	}
};


#endif //checkArraySizeH
