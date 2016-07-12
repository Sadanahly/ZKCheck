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
// Function Parameter checks
//---------------------------------------------------------------------------


#include "checkfuncparavoid.h"
#include "symboldatabase.h"
#include "tokenize.h"
#include "token.h"


// Register this check class into cppcheck by creating a static instance of it..
namespace {
	static CheckFuncParaVoid instance;
}

static const CWE CWE398(398U);  // Indicator of Poor Code Quality

void CheckFuncParaVoid::funcParaVoid() {
	const bool printWarning = _settings->isEnabled("warning");
	if (!printWarning)
		return;

	const SymbolDatabase *symbolDataBase = _tokenizer->getSymbolDatabase();
	std::list<Scope> scopeLists = symbolDataBase->scopeList;

	//function with void parameter report error when ii's defined and declaration in class/struct/namespace/global/union”Ú÷–
	std::list<Scope>::iterator iter = scopeLists.begin();
	for (; iter != scopeLists.end(); iter++) {
		if ((*iter).type == Scope::ScopeType::eClass || (*iter).type == Scope::ScopeType::eGlobal ||
			(*iter).type == Scope::ScopeType::eNamespace || (*iter).type == Scope::ScopeType::eStruct
			|| (*iter).type == Scope::ScopeType::eUnion) {
			std::list<Function> functionLists = (*iter).functionList;
			std::list<Function>::iterator funIter = functionLists.begin();
			for (; funIter != functionLists.end(); funIter++) {	
				//not test main and constructor and destructor function					 
				//test function declaration in global scope need loop all token
				if ((*iter).type == Scope::ScopeType::eGlobal) {
					const Token* tok = _tokenizer->list.front();
					for (; tok; tok = tok->next()) {
						if (tok->str() == "{") { //skip all scope for function declaration not in a scope
							tok = tok->link();
						}else if (tok->str() == "main") {
							continue;
						}else  if ((Token::Match(tok, "%name% ( ) ")) && (Token::Match(tok->next()->link()->next(), "const| ;|{"))) {
							if (isFunctionDecl(tok))
								errorFunctionPatameter(tok);
							else if(Token::Match(tok->next()->link()->next(), "const| {"))
								errorFunctionPatameter(tok);
						}
					}
				}
				else {
					//test function declaration in scope except global
					for (const Token* tok = (*iter).classStart; tok && tok != (*iter).classEnd; tok = tok->next()) {
						if ((Token::Match(tok, "%name% ( )") && (Token::Match(tok->next()->link()->next(), "const| ;")))) {
							std::multimap<std::string, const Function *>::const_iterator it = (*iter).functionMap.find(tok->str());
							if (it->second) { //function in the scope
								const Function *func = it->second;
								if (func->isConstructor() || func->isDestructor()) //not test ctor and dctor
									continue;

								if (isFunctionDecl(tok))
									errorFunctionPatameter(tok);
								else if (Token::Match(tok->next()->link()->next(), "const| {"))
									errorFunctionPatameter(tok);
							}
						}
					}
				}
				 
				}
			}
		}
	}
	 
void CheckFuncParaVoid::errorFunctionPatameter(const Token *tok) {
	reportError(tok,
		Severity::warning,
		"FunctionParameterVoid",
		"function without parameter should add parameter void", CWE398, false);
}

bool CheckFuncParaVoid::isFunctionDecl(const Token *tok) {
	const Token *curTok = tok;
	bool isDecl = true;

	//test there is no return value when function is called
	if (tok->previous()->str() == "[;{}]") {
		isDecl = false;
		return isDecl;
	}

	//test there is a return value when function is called
	for (; curTok && Token::Match(curTok, "[;}]"); curTok = curTok->previous()) {

		//when there is a return value test if function call return or assign 
		if ((Token::Match(tok, "%name%") && curTok->varId()) || Token::Match(tok, "return")) {
			isDecl = false;
			break;
		}
	}
	return isDecl;
}