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
// Switch Condition checks
//---------------------------------------------------------------------------

#include "checkswitchcondition.h"
#include "symboldatabase.h"
#include "tokenize.h"
#include "token.h"

// Register this check class into cppcheck by creating a static instance of it..
namespace {
	static CheckSwitchCondition instance;
}

static const CWE CWE398(398U);  // Indicator of Poor Code Quality

void CheckSwitchCondition::switchCondition() {
	const bool printWarning = _settings->isEnabled("warning");
	if (!printWarning)
		return;

	const SymbolDatabase *symbolDataBase = _tokenizer->getSymbolDatabase();
	std::list<Scope> scopesList = symbolDataBase->scopeList;

	//find all switch scope and then test if matched 
	std::list<Scope>::const_iterator iter = scopesList.begin();
	for (; iter != scopesList.end(); iter++){
		if ((*iter).type == Scope::ScopeType::eSwitch) {
			bool isDefaultExist = false;
			for (const Token *tok = (*iter).classEnd->previous(); tok && tok != (*iter).classStart; tok = tok->previous()) {
				if (tok->str() == "}") { //skip {} scope
					tok = tok->link();
					continue;
				}
				if (tok->str() == "default") { //find "default" key word
					isDefaultExist = true;
					break;
				}
			}

			if (!isDefaultExist) {
				const Token *tok = (*iter).classStart;
				while (tok->str() != "switch") {
					tok = tok->previous();
				}
				errorSwitchCondition(tok);
			}
		}
	}

	//find all might case in all functions scope
//	for (std::size_t i = 0; i < scopes; i++) {
//		const Scope *scope = symbolDataBase->functionScopes[i];
//
//		//reverse find if exists "default" and skip when meet scope
//		bool isDefaultExist = false;
//		for (const Token *tok = scope->classEnd; tok && tok != scope->classStart; tok = tok->previous()) {
//			if (tok->str() == "}") {
//				tok = tok->link();
//				continue;
//			}
//
//			if (tok->str() == "default") {
//				isDefaultExist = true;
//				
//			}			
//		}
//
//		
//	}
}

bool CheckSwitchCondition::isCircleStatement(const Token **tok) {	 //test if exist circle statement, exist then skip and find the end scope

	if (Token::Match(*tok, "do {") || (Token::Match(*tok, "for|while|switch (") && Token::simpleMatch((*tok)->next()->link(), ") {"))) {
		if ((*tok)->str() == "do") {
			(*tok) = (*tok)->next()->link()->next();
			(*tok) = (*tok)->next()->link()->next();
		}
		else {
			(*tok) = (*tok)->next()->link()->next()->link();
		}
		return true;
	}
	else {
		return false;
	}
}

bool CheckSwitchCondition::isExistBreak(const Token *start, const Token *end) {
	const Token *startTok = start, *tmpTok = start;
	int countOfIfElseScope = 0, countOfBreak = 0;
	while (tmpTok != end) {
		if ((Token::Match(tmpTok, "if (") && Token::simpleMatch(tmpTok->next()->link(), ") {"))
			|| (Token::Match(tmpTok, "else {"))) {
			countOfIfElseScope++;
			if (tmpTok->str() == "if") {
				tmpTok = tmpTok->next()->link()->next();
			}
			else {
				tmpTok = tmpTok->next();
			}

			const Token *beginTok = tmpTok, *endTok = tmpTok->link();
			tmpTok = endTok;
			for (const Token *tok = beginTok; tok != endTok; tok = tok->next()) {
				if (isCircleStatement(&tok)) { //skip circle statement in if statement
					continue;
				}
				else if (tok->str() == "break") {
					countOfBreak++;
				}
			}
		}
		else if (isCircleStatement(&tmpTok)) { //skip circle statement
			continue;
		}
		else { //not circle statement and not if...else statement then test if tok->str() == "break"
			if (tmpTok->str() == "break")
				return true;
			tmpTok = tmpTok->next();
		}
	}

	if (countOfIfElseScope) {
		if (countOfBreak == countOfIfElseScope)
			return true;
		else
			return false;
	}
	else {
		return false;
	}
	
}

void CheckSwitchCondition::caseDefaultCondition() {
	const bool printWarning = _settings->isEnabled("warning");
	if (!printWarning) {
		return;
	}

	const SymbolDatabase *symbolDataBase = _tokenizer->getSymbolDatabase();
	std::list<Scope> scopesList = symbolDataBase->scopeList;

	//find all switch scope and then test
	std::list<Scope>::const_iterator iter = scopesList.begin();
	for (; iter != scopesList.end(); iter++) {
		if ((*iter).type == Scope::ScopeType::eSwitch) {
			const Token *tok = (*iter).classStart;
			while(tok != (*iter).classEnd) {
				if (tok->str() == "case") { //begin test when find case
					const Token *caseTok = tok, *tmpTok = tok;					
					while (tmpTok->str() != ":")
						tmpTok = tmpTok->next();

					tmpTok = tmpTok->next();
					if (tmpTok->str() != ";") {		 //test if this case execute				
						tok = tmpTok;
						continue;
					}
					const Token *startTok = tmpTok;
					while (!Token::Match(tmpTok, "case|default") && tmpTok != (*iter).classEnd) { //test between two case or case and default or case and clssEnd
						if (isCircleStatement(&tmpTok)) //skip circle statement
							continue;
						tmpTok = tmpTok->next();
					}
					const Token *endTok = tmpTok; //find the end scope for the test
					if(!isExistBreak(startTok, endTok)){ //start test if exist break
						errorCaseBreakFault(caseTok);
					}
					tok = tmpTok;
					continue;
				}
				else { //continue next tok to test
					tok = tok->next();
				}
			}
		}
	}
}

void CheckSwitchCondition::errorCaseBreakFault(const Token *tok) {
	reportError(tok,
		Severity::error,
		"SwitchConditionError",
		"there is no break to match case!", CWE398, false);
}

void CheckSwitchCondition::errorSwitchCondition(const Token *tok) {
	reportError(tok,
		Severity::warning,
		"SwitchConditionError",
		"there is no default to match switch!", CWE398, false);
}