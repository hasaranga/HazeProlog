
// HazeProlog - Prolog like implementation for embedded systems
// (c) 2018 R.Hasaranga

/* 
Syntax limitations:
	(#) fact definition can only have constants. no variables!
	(#) max 2 terms within fact
	(#) max 2 facts within body of rule

Optimization notes:
	(#) define NO_RECURSIVE_RULES if you don't have rules with their body containing their own name. 
		it will remove "readLock" of Rule struct and allow you to define ROM-able(static const) rules!
	(#) define NO_ALL_VAR_QUERIES if you don't have queries with two variables.
	(#) define NO_OR_RULES if you don't have rules with OR operator.
*/

#include <stdio.h> // for printf
#include <string.h> // for strcmp

// change following two values according to your rules/facts definitions.
// consider stack size of your system when changing these values!
// (you can use MONITOR_BUFFERS definition to calculate minimum values.)
#define MAX_MATCHING_FACTS 5
#define MAX_MATCHING_RULES 5

// define MONITOR_BUFFERS if you want to display buffer usages.
// check buffer usage for each of your query. then you can set minimum values for MAX_MATCHING_FACTS and MAX_MATCHING_RULES.
#ifdef MONITOR_BUFFERS
#define PRINT_BUFFER_USAGE(USAGE,MAX) printf("buffer: %d / %d\n", USAGE, MAX); // replace this line according to your system!
#else
#define PRINT_BUFFER_USAGE(USAGE,MAX) 
#endif

bool StringCompare(const char *str1, const char* str2)
{
	return (strcmp(str1, str2) == 0); // replace this line according to your system!
}

bool IsCapitalLetter(char x)
{
	return (x >= 'A' && x <= 'Z');
}

bool IsVariable(const char *text)
{
	return IsCapitalLetter(text[0]);
}

struct Fact
{
	char termCount;
	const char *predicateName;
	bool isTerm1Var;
	const char *term1Name;
	bool isTerm2Var;
	const char *term2Name;

	const Fact *nextFact;
};

void FixVariableFlags(Fact *fact)
{
	fact->isTerm1Var = IsVariable(fact->term1Name);

	if (fact->termCount == 2)
		fact->isTerm2Var = IsVariable(fact->term2Name);
}

struct Rule
{
	Fact head;
	char factCountInBody;
	Fact fact1;
	bool op1IsAnd; // false if OR
	Fact fact2;

	const Rule *nextRule;

#ifndef NO_RECURSIVE_RULES // for ROM-able rules
	mutable bool readLock;
#endif
};

void CopyFact(Fact *output, const Fact *input)
{
	*output = *input;
	/*output->termCount = input->termCount;
	output->predicateName = input->predicateName;
	output->isTerm1Var = input->isTerm1Var;
	output->term1Name = input->term1Name;
	output->isTerm2Var = input->isTerm2Var;
	output->term2Name = input->term2Name;
	output->nextFact = input->nextFact;*/
}

void CopyRule(const Rule *input, Rule *output)
{
	*output = *input;
	/*CopyFact(&output->head, &input->head);
	output->factCountInBody = input->factCountInBody;
	CopyFact(&output->fact1, &input->fact1);
	output->op1IsAnd = input->op1IsAnd;
	CopyFact(&output->fact2, &input->fact2);
	output->nextRule = input->nextRule;*/
}

int GetVariableCountOfQuery(const Fact *query)
{
	int varCount = 0;

	if (query->isTerm1Var)
		++varCount;

	if ((query->termCount == 2) && (query->isTerm2Var))
		++varCount;

	return varCount;
}

// assume: termCount & predicateName matches
// fact can contain variables if it is from rule head.
bool IsFactMatch(const Fact *query, const Fact *fact)
{
	if (query->termCount == 1)
	{
		return query->isTerm1Var ? true : (fact->isTerm1Var ? true : StringCompare(query->term1Name, fact->term1Name));
	}
	else if (query->termCount == 2)
	{
		if (fact->isTerm1Var && fact->isTerm2Var) // both terms are variables in "fact"
			return true;

		if (query->isTerm1Var && query->isTerm2Var) // both terms are variables in "query"
		{
			if (StringCompare(query->term1Name, query->term2Name)) // pred(X , X)
				return StringCompare(fact->term1Name, fact->term2Name);
			else // pred(X , Y)
				return !StringCompare(fact->term1Name, fact->term2Name);
		}
		else if (query->isTerm1Var && (!query->isTerm2Var)) // term2 is constant in query
		{
			return fact->isTerm2Var ? true : StringCompare(query->term2Name, fact->term2Name);
		}
		else if ((!query->isTerm1Var) && query->isTerm2Var) // term1 is constant in query
		{
			return fact->isTerm1Var ? true : StringCompare(query->term1Name, fact->term1Name);
		}
		else // both constant in query
		{
			int varCount = GetVariableCountOfQuery(fact);

			if (varCount == 0) // fact has no variables
			{
				return StringCompare(query->term1Name, fact->term1Name) && StringCompare(query->term2Name, fact->term2Name);
			}
			else if (varCount == 1) // fact has one variable
			{
				if (fact->isTerm1Var)
					return StringCompare(query->term2Name, fact->term2Name);
				else
					return StringCompare(query->term1Name, fact->term1Name);
			}
		}
	}

	return false;
}

bool FindMatchingFactsFromFactList(const Fact *query, const Fact *firstFact, int *factCount, const Fact **result)
{
	const Fact *nextFact = firstFact;
	*factCount = 0;

	while (nextFact)
	{
		if ((nextFact->termCount == query->termCount)
			&& StringCompare(nextFact->predicateName, query->predicateName)
			&& IsFactMatch(query, nextFact))
		{
			result[*factCount] = nextFact;
			++(*factCount);
		}
		nextFact = nextFact->nextFact;
	}

	return ((*factCount) != 0);
}

void FindMatchingRulesFromRulesList(const Fact *query, const Rule *firstRule, int *ruleCount, const Rule **result)
{
	const Rule *nextRule = firstRule;
	*ruleCount = 0;

	while (nextRule)
	{
#ifndef NO_RECURSIVE_RULES
		if ( (!nextRule->readLock) 
			&& (nextRule->head.termCount == query->termCount)
			&& StringCompare(nextRule->head.predicateName, query->predicateName)
			&& IsFactMatch(query, &nextRule->head))
#else  
		if ( (nextRule->head.termCount == query->termCount)
			&& StringCompare(nextRule->head.predicateName, query->predicateName)
			&& IsFactMatch(query, &nextRule->head))
#endif
		{
			result[*ruleCount] = nextRule;
			++(*ruleCount);
		}
		nextRule = nextRule->nextRule;
	}
}

void ReplaceVariablesInRule(const Fact *query, const Rule *input, Rule *output)
{
	output->head.term1Name = query->term1Name;
	output->head.isTerm1Var = query->isTerm1Var;

	if (query->termCount == 2)
	{
		output->head.term2Name = query->term2Name;
		output->head.isTerm2Var = query->isTerm2Var;
	}

	if (input->head.isTerm1Var && input->fact1.isTerm1Var && StringCompare(input->head.term1Name, input->fact1.term1Name))
	{
		output->fact1.term1Name = output->head.term1Name;
		output->fact1.isTerm1Var = output->head.isTerm1Var;
	}

	if (input->head.isTerm1Var && (input->fact1.termCount == 2) && input->fact1.isTerm2Var && StringCompare(input->head.term1Name, input->fact1.term2Name))
	{
		output->fact1.term2Name = output->head.term1Name;
		output->fact1.isTerm2Var = output->head.isTerm1Var;
	}

	if (input->factCountInBody == 2)
	{
		if (input->head.isTerm1Var && input->fact2.isTerm1Var && StringCompare(input->head.term1Name, input->fact2.term1Name))
		{
			output->fact2.term1Name = output->head.term1Name;
			output->fact2.isTerm1Var = output->head.isTerm1Var;
		}

		if (input->head.isTerm1Var && (input->fact2.termCount == 2) && input->fact2.isTerm2Var && StringCompare(input->head.term1Name, input->fact2.term2Name))
		{
			output->fact2.term2Name = output->head.term1Name;
			output->fact2.isTerm2Var = output->head.isTerm1Var;
		}
	}

	if (query->termCount == 2)
	{
		if (input->head.isTerm2Var && input->fact1.isTerm1Var && StringCompare(input->head.term2Name, input->fact1.term1Name))
		{
			output->fact1.term1Name = output->head.term2Name;
			output->fact1.isTerm1Var = output->head.isTerm2Var;
		}

		if (input->head.isTerm2Var && (input->fact1.termCount == 2) && input->fact1.isTerm2Var && StringCompare(input->head.term2Name, input->fact1.term2Name))
		{
			output->fact1.term2Name = output->head.term2Name;
			output->fact1.isTerm2Var = output->head.isTerm2Var;
		}

		if (input->factCountInBody == 2)
		{
			if (input->head.isTerm2Var && input->fact2.isTerm1Var && StringCompare(input->head.term2Name, input->fact2.term1Name))
			{
				output->fact2.term1Name = output->head.term2Name;
				output->fact2.isTerm1Var = output->head.isTerm2Var;
			}

			if (input->head.isTerm2Var && (input->fact2.termCount == 2) && input->fact2.isTerm2Var && StringCompare(input->head.term2Name, input->fact2.term2Name))
			{
				output->fact2.term2Name = output->head.term2Name;
				output->fact2.isTerm2Var = output->head.isTerm2Var;
			}
		}
	}
}

// assume: intput list does not contain vars
// if query has one var then first term of output is the result
// if query has two vars or no vars then output is same as input
void PutResultsAccordingToQuery(const Fact *query, const Fact **inputList, int inputListSize, Fact *outputList, int *outputListCurrentIndex)
{
	int varCount = GetVariableCountOfQuery(query);

	for (int i = 0; i < inputListSize; ++i)
	{
		CopyFact(&outputList[*outputListCurrentIndex], inputList[i]);

		if (varCount == 1)
		{
			if (!query->isTerm1Var) // assign term2 into term1	 
			{
				outputList[*outputListCurrentIndex].term1Name = outputList[*outputListCurrentIndex].term2Name;
				outputList[*outputListCurrentIndex].isTerm1Var = false;
			}
		}

		++(*outputListCurrentIndex);
	}
}

bool SolveQuery(const Fact *query, const Rule *firstRule, const Fact *firstFact, int *resultCount, Fact *results)
{
	int matchingRulesCount;
	const Rule *matchingRules[MAX_MATCHING_RULES];
	FindMatchingRulesFromRulesList(query, firstRule, &matchingRulesCount, matchingRules);

	PRINT_BUFFER_USAGE(matchingRulesCount, MAX_MATCHING_RULES)

	if (matchingRulesCount) // do we have matching rules?
	{
		Fact fact1Results[MAX_MATCHING_FACTS];

		#ifndef NO_ALL_VAR_QUERIES
		Fact fact2Results[MAX_MATCHING_FACTS]; // we put this in here. because it was declared multiple times within different blocks and caused higher(3x) stack usage!
		#endif

		bool found = false;
		for (int i = 0; i < matchingRulesCount; ++i)
		{
			const Rule *matchingRule = matchingRules[i];

			#ifndef NO_RECURSIVE_RULES
			matchingRule->readLock = true; // acquire lock
			#endif

			Rule queringRule;
			CopyRule(matchingRule, &queringRule);
			ReplaceVariablesInRule(query, matchingRule, &queringRule);

			// make the fact who has single variable as first fact of queringRule
			if ((queringRule.factCountInBody == 2) && (queringRule.op1IsAnd))
			{
				int fact1VariableCount = GetVariableCountOfQuery(&queringRule.fact1);
				if (fact1VariableCount == 2) // exchange fact1 with fact2
				{
					Fact tmp;
					CopyFact(&tmp, &queringRule.fact1);
					CopyFact(&queringRule.fact1, &queringRule.fact2);
					CopyFact(&queringRule.fact2, &tmp);
				}
			}

			int resultCountForFact1 = 0;		
			bool hasResults = SolveQuery(&queringRule.fact1, firstRule, firstFact, &resultCountForFact1, fact1Results);		

			PRINT_BUFFER_USAGE(resultCountForFact1, MAX_MATCHING_FACTS)

			if (queringRule.factCountInBody == 1)
			{
				for (int j = 0; j < resultCountForFact1; ++j) // add first fact results
				{
					CopyFact(&results[(*resultCount)], &fact1Results[j]);
					++(*resultCount);
				}
			}
			#ifndef NO_OR_RULES
			else if ((queringRule.factCountInBody == 2) && (!queringRule.op1IsAnd)) // OR with second fact
			{
				for (int j = 0; j < resultCountForFact1; ++j) // add first fact results
				{
					CopyFact(&results[(*resultCount)], &fact1Results[j]);
					++(*resultCount);
				}

				int resultCountForFact2 = 0;				
				bool hasResults2 = SolveQuery(&queringRule.fact2, firstRule, firstFact, &resultCountForFact2, fact1Results); // we can use same buffer(fact1Results) to store results.

				PRINT_BUFFER_USAGE(resultCountForFact2, MAX_MATCHING_FACTS)

				for (int j = 0; j < resultCountForFact2; ++j) // add second fact results
				{
					CopyFact(&results[(*resultCount)], &fact1Results[j]);
					++(*resultCount);
				}

				hasResults |= hasResults2;
			}
			#endif
			else if (hasResults && (queringRule.factCountInBody == 2) && (queringRule.op1IsAnd)) // AND with second fact
			{
				int fact1VariableCount = GetVariableCountOfQuery(&queringRule.fact1);
				bool hasResults2 = false;

				for (int j = 0; j < resultCountForFact1; ++j) // check each fact1 results
				{
					Fact queringFact;
					CopyFact(&queringFact, &queringRule.fact2);

					// replace queringFact variables with fact1 result
					if (queringRule.fact1.isTerm1Var)
					{
						if (StringCompare(queringRule.fact1.term1Name, queringFact.term1Name))
						{
							queringFact.term1Name = fact1Results[j].term1Name;
							queringFact.isTerm1Var = false;
						}
						else if ((queringFact.termCount == 2) && StringCompare(queringRule.fact1.term1Name, queringFact.term2Name))
						{
							queringFact.term2Name = fact1Results[j].term1Name;
							queringFact.isTerm2Var = false;
						}
					}

					if ((queringRule.fact1.termCount == 2) && queringRule.fact1.isTerm2Var)
					{
						if (StringCompare(queringRule.fact1.term2Name, queringFact.term1Name))
						{
							queringFact.term1Name = fact1Results[j].term1Name;
							queringFact.isTerm1Var = false;
						}
						else if ((queringFact.termCount == 2) && StringCompare(queringRule.fact1.term2Name, queringFact.term2Name))
						{
							queringFact.term2Name = fact1Results[j].term1Name;
							queringFact.isTerm2Var = false;
						}
					}

					// solve queringFact
					if (fact1VariableCount == 1) // first fact has one variable
					{
						if (GetVariableCountOfQuery(&queringRule.head) == 1) // output require one column results
						{
							hasResults2 |= SolveQuery(&queringFact, firstRule, firstFact, resultCount, results);
						}
						else // output require 2 column results. So, we need to re-arrange results according to query. Ex: "query(X,Y)" and we have "rule(X,Y) = fact1(X) , fact2(X,Y)"
						{
							#ifndef NO_ALL_VAR_QUERIES

							int resultCountForFact2 = 0;
							hasResults2 |= SolveQuery(&queringFact, firstRule, firstFact, &resultCountForFact2, fact2Results);

							PRINT_BUFFER_USAGE(resultCountForFact2, MAX_MATCHING_FACTS)

							for (int k = 0; k < resultCountForFact2; ++k) // re-arrange results
							{
								if (StringCompare(queringRule.head.term1Name, queringRule.fact1.term1Name)) // rule(X,Y) = fact1(X) , fact2(?,?)
								{
									results[(*resultCount)].term1Name = fact1Results[j].term1Name;

									if (StringCompare(queringRule.head.term1Name, queringRule.fact2.term1Name)) // rule(X,Y) = fact1(X) , fact2(X,Y)				
										results[(*resultCount)].term2Name = fact2Results[k].term2Name;
									else // rule(X,Y) = fact1(X) , fact2(Y,X)
										results[(*resultCount)].term2Name = fact2Results[k].term1Name;
								}
								else // rule(Y,X) = fact1(X) , fact2(?,?)
								{
									results[(*resultCount)].term2Name = fact1Results[j].term1Name;

									if (StringCompare(queringRule.head.term1Name, queringRule.fact2.term1Name)) // rule(Y,X) = fact1(X) , fact2(X,Y)				
										results[(*resultCount)].term1Name = fact2Results[k].term2Name;
									else // rule(Y,X) = fact1(X) , fact2(Y,X)
										results[(*resultCount)].term1Name = fact2Results[k].term1Name;
								}
								++(*resultCount);
							}

							#endif
						}
					}
					else if (fact1VariableCount == 2) // both facts has 2 variables. Ex: "query(X,Y)" and we have "rule(X,Y) = fact1(X,Z) , fact2(Z,Y)"
					{
						#ifndef NO_ALL_VAR_QUERIES

						int resultCountForFact2 = 0;
						hasResults2 |= SolveQuery(&queringFact, firstRule, firstFact, &resultCountForFact2, fact2Results);

						PRINT_BUFFER_USAGE(resultCountForFact2, MAX_MATCHING_FACTS)

						for (int k = 0; k < resultCountForFact2; ++k) // re-arrange results
						{
							if (StringCompare(queringRule.head.term1Name, queringRule.fact1.term1Name)) // rule(X,Y) = fact1(X, ?) , fact2(?,?)
							{
								results[(*resultCount)].term1Name = fact1Results[j].term1Name;

								if (StringCompare(queringRule.head.term2Name, queringRule.fact2.term1Name)) // rule(X,Y) = fact1(X, ?) , fact2(Y,?)				
									results[(*resultCount)].term2Name = fact2Results[k].term1Name;
								else // rule(X,Y) = fact1(X, ?) , fact2(?,Y)		
									results[(*resultCount)].term2Name = fact2Results[k].term2Name;
							}
							else if (StringCompare(queringRule.head.term1Name, queringRule.fact1.term2Name)) // rule(X,Y) = fact1(?, X) , fact2(?,?)
							{
								results[(*resultCount)].term1Name = fact1Results[j].term2Name;

								if (StringCompare(queringRule.head.term2Name, queringRule.fact2.term1Name)) // rule(X,Y) = fact1(?, X) , fact2(Y,?)				
									results[(*resultCount)].term2Name = fact2Results[k].term1Name;
								else // rule(X,Y) = fact1(?, X) , fact2(?,Y)		
									results[(*resultCount)].term2Name = fact2Results[k].term2Name;
							}
							else if (StringCompare(queringRule.head.term1Name, queringRule.fact2.term1Name)) // rule(X,Y) = fact1(?, ?) , fact2(X,?)
							{
								results[(*resultCount)].term1Name = fact2Results[k].term1Name;

								if (StringCompare(queringRule.head.term2Name, queringRule.fact1.term1Name)) // rule(X,Y) = fact1(Y, ?) , fact2(X,?)				
									results[(*resultCount)].term2Name = fact1Results[j].term1Name;
								else  // rule(X,Y) = fact1(?, Y) , fact2(X,?)			
									results[(*resultCount)].term2Name = fact1Results[j].term2Name;
							}
							else  // rule(X,Y) = fact1(?, ?) , fact2(?,X)
							{
								results[(*resultCount)].term1Name = fact2Results[k].term2Name;

								if (StringCompare(queringRule.head.term2Name, queringRule.fact1.term1Name)) // rule(X,Y) = fact1(Y, ?) , fact2(?,X)				
									results[(*resultCount)].term2Name = fact1Results[j].term1Name;
								else  // rule(X,Y) = fact1(?, Y) , fact2(?,X)			
									results[(*resultCount)].term2Name = fact1Results[j].term2Name;
							}
							++(*resultCount);
						}

						#endif
					}
				}

				hasResults &= hasResults2;
			}

			#ifndef NO_RECURSIVE_RULES
			matchingRule->readLock = false; // release lock
			#endif

			found |= hasResults;
		}

		return found;
	}
	else // search in facts list
	{
		int matchingFactCount;
		const Fact *matchingFacts[MAX_MATCHING_FACTS];
		bool hasResults = FindMatchingFactsFromFactList(query, firstFact, &matchingFactCount, matchingFacts);

		PRINT_BUFFER_USAGE(matchingFactCount, MAX_MATCHING_FACTS)

		if (hasResults)
			PutResultsAccordingToQuery(query, matchingFacts, matchingFactCount, results, resultCount);

		return hasResults;
	}

}

// if query has one var then print first term of result
// if query has two vars then print both terms of result
// if query has no vars then print true
void PrintResultAccordingToQuery(Fact *query, Fact *result) // replace printf function according to your system!
{
	int varCount = GetVariableCountOfQuery(query);

	if (varCount == 0)
	{
		printf("true");
	}
	else if (varCount == 1)
	{
		printf(result->term1Name);
	}
	else if (varCount == 2)
	{
		printf(result->term1Name);
		printf(", ");
		printf(result->term2Name);
	}

	printf("\n");
}
