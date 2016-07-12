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


//---------------------------------------------------------------------------
// Array Size checks
//---------------------------------------------------------------------------

#include "checkArraySize.h"
#include "symboldatabase.h"


// Register this check class into cppcheck by creating a static instance of it..
namespace {
	static CheckArraySize instance;
}

static const CWE CWE398(398U);  // Indicator of Poor Code Quality

void CheckArraySize::allocArraySize() {
	const bool printWarning = _settings->isEnabled("warning");
	if (!printWarning)
		return;

	//const SymbolDatabase *symbolDataBase = _tokenizer->getSymbolDatabase();
	//const std::size_t functions = symbolDataBase->functionScopes.size();
	/*for (std::size_t i = 0; i < functions; i++) {
		const Scope *scope = symbolDataBase->functionScopes[i];
		for (const Token *tok = scope->classStart; tok && tok != scope->classEnd; tok = tok->next()) {
			if ((Token::Match(tok, "%var% [ %num% ] = { %str% } ;") || Token::Match(tok, "%var% [ %num% ] = %str% ;") 
				&& tok->previous()->str() == "char")) {					 
				errorArraySizeAllocated(tok);
			}
		}
	}*/

	const Token* tok = _tokenizer->list.front();
	for (; tok; tok = tok->next()) {
		if ((Token::Match(tok, "%name% [ %num% ] = { %str% } ;") || Token::Match(tok, "%name% [ %num% ] = %str% ;")
			&& tok->previous()->str() == "char")) {
			errorArraySizeAllocated(tok);
		}
	}
}

void CheckArraySize::errorArraySizeAllocated(const Token *tok) {
	reportError(tok,
		Severity::warning,
		"allocArraySize",
		"the size of char Array is not standard", CWE398, false);
}