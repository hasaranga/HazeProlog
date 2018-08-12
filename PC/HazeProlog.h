
// HazeProlog - Prolog like implementation for embedded systems
// (c) 2018 R.Hasaranga

/* 
Syntax limitations:
	(#) fact definition can only have constants. no variables!
	(#) max 2 terms within fact
	(#) max 2 facts within body of rule

Optimization notes:
	(#) define NO_RECURSIVE_RULES if you don't have rules with their body containing their own name. 
		it will remove "readLock" of Rule struct and allow you to define static const rules!
	(#) define NO_ALL_VAR_QUERIES if you don't have queries with two variables.
	(#) define NO_OR_RULES if you don't have rules with OR operator.
*/

#ifndef HAZE_PROLOG_H_
#define HAZE_PROLOG_H_

#include <string.h> // for strcmp

#ifndef Arduino_h
#include <stdio.h> // for printf
#endif

#ifdef PRINT_FREE_MEM
#include <MemoryFree.h> // for AVR free mem
#endif

#ifdef Arduino_h
#define PRINT(TXT) Serial.print(TXT)
#else
#define PRINT(TXT) printf(TXT)
#endif

// change following two values according to your rules/facts definitions.
// consider stack size of your system when changing these values!
// (you can use MONITOR_BUFFERS definition to calculate minimum values.)
#define MAX_MATCHING_FACTS 5
#define MAX_MATCHING_RULES 5

// define MONITOR_BUFFERS if you want to display buffer usages.
// check buffer usage for each of your query. then you can set minimum values for MAX_MATCHING_FACTS and MAX_MATCHING_RULES.
#ifdef MONITOR_BUFFERS
#define PRINT_BUFFER_USAGE(USAGE,MAX) PRINT("buffer: "); PRINT((int)USAGE); PRINT(" / "); PRINT((int)MAX); PRINT("\n");
#else
#define PRINT_BUFFER_USAGE(USAGE,MAX) 
#endif

#ifndef int8
#define int8 char
#endif

#ifndef NO_INLINE
#ifdef __GNUG__ // g++
#define NO_INLINE __attribute__((noinline))
#else
#define NO_INLINE 
#endif
#endif

struct Fact
{
	int8 termCount;
	const char *predicateName;
	bool isTerm1Var;
	const char *term1Name;
	bool isTerm2Var;
	const char *term2Name;

	const Fact *nextFact;
};

struct Rule
{
	Fact head;
	int8 factCountInBody;
	Fact fact1;
	bool op1IsAnd; // false if OR
	Fact fact2;

	const Rule *nextRule;

#ifndef NO_RECURSIVE_RULES // for ROM-able rules
	mutable bool readLock;
#endif
};

class HazeProlog
{
protected:
	const Rule *firstRule;
	const Fact *firstFact;

public:

	void SetRuleFactDefinitions(const Rule *firstRule, const Fact *firstFact)
	{
		this->firstFact = firstFact;
		this->firstRule = firstRule;
	}

	static bool StringCompare(const char *str1, const char* str2)
	{
		return (::strcmp(str1, str2) == 0); // replace this line according to your system!
	}

	static bool IsCapitalLetter(const char x)
	{
		return ( (x >= 'A') && (x <= 'Z') );
	}

	static bool IsVariable(const char *text)
	{
		return HazeProlog::IsCapitalLetter(text[0]);
	}

	static void FixVariableFlags(Fact *fact)
	{
		fact->isTerm1Var = HazeProlog::IsVariable(fact->term1Name);

		if (fact->termCount == 2)
			fact->isTerm2Var = HazeProlog::IsVariable(fact->term2Name);
	}

	static void CopyFact(Fact *output, const Fact *input)
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

	static void CopyRule(const Rule *input, Rule *output)
	{
		*output = *input;
		/*CopyFact(&output->head, &input->head);
		output->factCountInBody = input->factCountInBody;
		CopyFact(&output->fact1, &input->fact1);
		output->op1IsAnd = input->op1IsAnd;
		CopyFact(&output->fact2, &input->fact2);
		output->nextRule = input->nextRule;*/
	}

	static char GetVariableCountOfQuery(const Fact *query)
	{
		int8 varCount = 0;

		if (query->isTerm1Var)
			++varCount;

		if ((query->termCount == 2) && (query->isTerm2Var))
			++varCount;

		return varCount;
	}

	// assume: termCount & predicateName matches
	// fact can contain variables if it is from rule head.
	static bool IsFactMatch(const Fact *query, const Fact *fact)
	{
		if (query->termCount == 1)
		{
			return query->isTerm1Var ? true : (fact->isTerm1Var ? true : HazeProlog::StringCompare(query->term1Name, fact->term1Name));
		}
		else if (query->termCount == 2)
		{
			if (fact->isTerm1Var && fact->isTerm2Var) // both terms are variables in "fact"
				return true;

			if (query->isTerm1Var && query->isTerm2Var) // both terms are variables in "query"
			{
				if (HazeProlog::StringCompare(query->term1Name, query->term2Name)) // pred(X , X)
					return HazeProlog::StringCompare(fact->term1Name, fact->term2Name);
				else // pred(X , Y)
					return !HazeProlog::StringCompare(fact->term1Name, fact->term2Name);
			}
			else if (query->isTerm1Var && (!query->isTerm2Var)) // term2 is constant in query
			{
				return fact->isTerm2Var ? true : HazeProlog::StringCompare(query->term2Name, fact->term2Name);
			}
			else if ((!query->isTerm1Var) && query->isTerm2Var) // term1 is constant in query
			{
				return fact->isTerm1Var ? true : HazeProlog::StringCompare(query->term1Name, fact->term1Name);
			}
			else // both constant in query
			{
				char varCount = HazeProlog::GetVariableCountOfQuery(fact);

				if (varCount == 0) // fact has no variables
				{
					return HazeProlog::StringCompare(query->term1Name, fact->term1Name) && HazeProlog::StringCompare(query->term2Name, fact->term2Name);
				}
				else if (varCount == 1) // fact has one variable
				{
					if (fact->isTerm1Var)
						return HazeProlog::StringCompare(query->term2Name, fact->term2Name);
					else
						return HazeProlog::StringCompare(query->term1Name, fact->term1Name);
				}
			}
		}

		return false;
	}

	bool FindMatchingFactsFromFactList(const Fact *query, int8 *factCount, const Fact **result)
	{
		const Fact *nextFact = firstFact;
		*factCount = 0;

		while (nextFact)
		{
			if ((nextFact->termCount == query->termCount)
				&& HazeProlog::StringCompare(nextFact->predicateName, query->predicateName)
				&& HazeProlog::IsFactMatch(query, nextFact))
			{
				result[*factCount] = nextFact;
				++(*factCount);
			}
			nextFact = nextFact->nextFact;
		}

		return ((*factCount) != 0);
	}

	void FindMatchingRulesFromRulesList(const Fact *query, int8 *ruleCount, const Rule **result)
	{
		const Rule *nextRule = firstRule;
		*ruleCount = 0;

		while (nextRule)
		{
#ifndef NO_RECURSIVE_RULES
			if ((!nextRule->readLock)
				&& (nextRule->head.termCount == query->termCount)
				&& HazeProlog::StringCompare(nextRule->head.predicateName, query->predicateName)
				&& HazeProlog::IsFactMatch(query, &nextRule->head))
#else  
			if ( (nextRule->head.termCount == query->termCount)
				&& HazeProlog::StringCompare(nextRule->head.predicateName, query->predicateName)
				&& HazeProlog::IsFactMatch(query, &nextRule->head))
#endif
			{
				result[*ruleCount] = nextRule;
				++(*ruleCount);
			}
			nextRule = nextRule->nextRule;
		}
	}

	static void ReplaceVariablesInRule(const Fact *query, const Rule *input, Rule *output)
	{
		output->head.term1Name = query->term1Name;
		output->head.isTerm1Var = query->isTerm1Var;

		if (query->termCount == 2)
		{
			output->head.term2Name = query->term2Name;
			output->head.isTerm2Var = query->isTerm2Var;
		}

		if (input->head.isTerm1Var && input->fact1.isTerm1Var && HazeProlog::StringCompare(input->head.term1Name, input->fact1.term1Name))
		{
			output->fact1.term1Name = output->head.term1Name;
			output->fact1.isTerm1Var = output->head.isTerm1Var;
		}

		if (input->head.isTerm1Var && (input->fact1.termCount == 2) && input->fact1.isTerm2Var && HazeProlog::StringCompare(input->head.term1Name, input->fact1.term2Name))
		{
			output->fact1.term2Name = output->head.term1Name;
			output->fact1.isTerm2Var = output->head.isTerm1Var;
		}

		if (input->factCountInBody == 2)
		{
			if (input->head.isTerm1Var && input->fact2.isTerm1Var && HazeProlog::StringCompare(input->head.term1Name, input->fact2.term1Name))
			{
				output->fact2.term1Name = output->head.term1Name;
				output->fact2.isTerm1Var = output->head.isTerm1Var;
			}

			if (input->head.isTerm1Var && (input->fact2.termCount == 2) && input->fact2.isTerm2Var && HazeProlog::StringCompare(input->head.term1Name, input->fact2.term2Name))
			{
				output->fact2.term2Name = output->head.term1Name;
				output->fact2.isTerm2Var = output->head.isTerm1Var;
			}
		}

		if (query->termCount == 2)
		{
			if (input->head.isTerm2Var && input->fact1.isTerm1Var && HazeProlog::StringCompare(input->head.term2Name, input->fact1.term1Name))
			{
				output->fact1.term1Name = output->head.term2Name;
				output->fact1.isTerm1Var = output->head.isTerm2Var;
			}

			if (input->head.isTerm2Var && (input->fact1.termCount == 2) && input->fact1.isTerm2Var && HazeProlog::StringCompare(input->head.term2Name, input->fact1.term2Name))
			{
				output->fact1.term2Name = output->head.term2Name;
				output->fact1.isTerm2Var = output->head.isTerm2Var;
			}

			if (input->factCountInBody == 2)
			{
				if (input->head.isTerm2Var && input->fact2.isTerm1Var && HazeProlog::StringCompare(input->head.term2Name, input->fact2.term1Name))
				{
					output->fact2.term1Name = output->head.term2Name;
					output->fact2.isTerm1Var = output->head.isTerm2Var;
				}

				if (input->head.isTerm2Var && (input->fact2.termCount == 2) && input->fact2.isTerm2Var && HazeProlog::StringCompare(input->head.term2Name, input->fact2.term2Name))
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
	static void PutResultsAccordingToQuery(const Fact *query, const Fact **inputList, int8 inputListSize, Fact *outputList, int8 *outputListCurrentIndex)
	{
		int8 varCount = HazeProlog::GetVariableCountOfQuery(query);

		for (int8 i = 0; i < inputListSize; ++i)
		{
			HazeProlog::CopyFact(&outputList[*outputListCurrentIndex], inputList[i]);

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

	NO_INLINE bool SolveFactQuery(const Fact *query, int8 *resultCount, Fact *results)
	{
#ifdef PRINT_FREE_MEM
		PRINT("SolveFactQuery Free Mem: ");
		PRINT(freeMemory());
		PRINT("\n");
#endif

		int8 matchingFactCount;
		const Fact *matchingFacts[MAX_MATCHING_FACTS];
		bool hasResults = this->FindMatchingFactsFromFactList(query, &matchingFactCount, matchingFacts);

		PRINT_BUFFER_USAGE(matchingFactCount, MAX_MATCHING_FACTS);

		if (hasResults)
			HazeProlog::PutResultsAccordingToQuery(query, matchingFacts, matchingFactCount, results, resultCount);

		return hasResults;
	}

	NO_INLINE void OptFunc1(bool *hasResults2, Fact *queringFact, Rule *queringRule, Fact *fact1Results, int8 *resultCount, Fact *results, int8 j)
	{
		Fact fact2Results[MAX_MATCHING_FACTS];
		int8 resultCountForFact2 = 0;

		(*hasResults2) |= this->SolveQuery(queringFact, &resultCountForFact2, fact2Results);

		PRINT_BUFFER_USAGE(resultCountForFact2, MAX_MATCHING_FACTS);

		for (int8 k = 0; k < resultCountForFact2; ++k) // re-arrange results
		{
			if (HazeProlog::StringCompare(queringRule->head.term1Name, queringRule->fact1.term1Name)) // rule(X,Y) = fact1(X) , fact2(?,?)
			{
				results[(*resultCount)].term1Name = fact1Results[j].term1Name;

				if (HazeProlog::StringCompare(queringRule->head.term1Name, queringRule->fact2.term1Name)) // rule(X,Y) = fact1(X) , fact2(X,Y)				
					results[(*resultCount)].term2Name = fact2Results[k].term2Name;
				else // rule(X,Y) = fact1(X) , fact2(Y,X)
					results[(*resultCount)].term2Name = fact2Results[k].term1Name;
			}
			else // rule(Y,X) = fact1(X) , fact2(?,?)
			{
				results[(*resultCount)].term2Name = fact1Results[j].term1Name;

				if (HazeProlog::StringCompare(queringRule->head.term1Name, queringRule->fact2.term1Name)) // rule(Y,X) = fact1(X) , fact2(X,Y)				
					results[(*resultCount)].term1Name = fact2Results[k].term2Name;
				else // rule(Y,X) = fact1(X) , fact2(Y,X)
					results[(*resultCount)].term1Name = fact2Results[k].term1Name;
			}
			++(*resultCount);
		}
	}

	NO_INLINE void OptFunc2(bool *hasResults2, Fact *queringFact, Rule *queringRule, Fact *fact1Results, int8 *resultCount, Fact *results, int8 j)
	{
		Fact fact2Results[MAX_MATCHING_FACTS];
		int8 resultCountForFact2 = 0;
		(*hasResults2) |= this->SolveQuery(queringFact, &resultCountForFact2, fact2Results);

		PRINT_BUFFER_USAGE(resultCountForFact2, MAX_MATCHING_FACTS);

		for (int8 k = 0; k < resultCountForFact2; ++k) // re-arrange results
		{
			if (HazeProlog::StringCompare(queringRule->head.term1Name, queringRule->fact1.term1Name)) // rule(X,Y) = fact1(X, ?) , fact2(?,?)
			{
				results[(*resultCount)].term1Name = fact1Results[j].term1Name;

				if (HazeProlog::StringCompare(queringRule->head.term2Name, queringRule->fact2.term1Name)) // rule(X,Y) = fact1(X, ?) , fact2(Y,?)				
					results[(*resultCount)].term2Name = fact2Results[k].term1Name;
				else // rule(X,Y) = fact1(X, ?) , fact2(?,Y)		
					results[(*resultCount)].term2Name = fact2Results[k].term2Name;
			}
			else if (HazeProlog::StringCompare(queringRule->head.term1Name, queringRule->fact1.term2Name)) // rule(X,Y) = fact1(?, X) , fact2(?,?)
			{
				results[(*resultCount)].term1Name = fact1Results[j].term2Name;

				if (HazeProlog::StringCompare(queringRule->head.term2Name, queringRule->fact2.term1Name)) // rule(X,Y) = fact1(?, X) , fact2(Y,?)				
					results[(*resultCount)].term2Name = fact2Results[k].term1Name;
				else // rule(X,Y) = fact1(?, X) , fact2(?,Y)		
					results[(*resultCount)].term2Name = fact2Results[k].term2Name;
			}
			else if (HazeProlog::StringCompare(queringRule->head.term1Name, queringRule->fact2.term1Name)) // rule(X,Y) = fact1(?, ?) , fact2(X,?)
			{
				results[(*resultCount)].term1Name = fact2Results[k].term1Name;

				if (HazeProlog::StringCompare(queringRule->head.term2Name, queringRule->fact1.term1Name)) // rule(X,Y) = fact1(Y, ?) , fact2(X,?)				
					results[(*resultCount)].term2Name = fact1Results[j].term1Name;
				else  // rule(X,Y) = fact1(?, Y) , fact2(X,?)			
					results[(*resultCount)].term2Name = fact1Results[j].term2Name;
			}
			else  // rule(X,Y) = fact1(?, ?) , fact2(?,X)
			{
				results[(*resultCount)].term1Name = fact2Results[k].term2Name;

				if (HazeProlog::StringCompare(queringRule->head.term2Name, queringRule->fact1.term1Name)) // rule(X,Y) = fact1(Y, ?) , fact2(?,X)				
					results[(*resultCount)].term2Name = fact1Results[j].term1Name;
				else  // rule(X,Y) = fact1(?, Y) , fact2(?,X)			
					results[(*resultCount)].term2Name = fact1Results[j].term2Name;
			}
			++(*resultCount);
		}
	}

	NO_INLINE bool SolveRuleQuery(const Fact *query, int8 *resultCount, Fact *results)
	{
#ifdef PRINT_FREE_MEM
		PRINT("SolveRuleQuery Free Mem: ");
		PRINT(freeMemory());
		PRINT("\n");
#endif

		int8 matchingRulesCount;
		const Rule *matchingRules[MAX_MATCHING_RULES];
		this->FindMatchingRulesFromRulesList(query, &matchingRulesCount, matchingRules);

		PRINT_BUFFER_USAGE(matchingRulesCount, MAX_MATCHING_RULES);

		if (matchingRulesCount)
		{
			Fact fact1Results[MAX_MATCHING_FACTS];

			bool found = false;
			for (int8 i = 0; i < matchingRulesCount; ++i)
			{
				const Rule *matchingRule = matchingRules[i];

#ifndef NO_RECURSIVE_RULES
				matchingRule->readLock = true; // acquire lock
#endif

				Rule queringRule;
				HazeProlog::CopyRule(matchingRule, &queringRule);
				HazeProlog::ReplaceVariablesInRule(query, matchingRule, &queringRule);

				// make the fact who has single variable as first fact of queringRule
				if ((queringRule.factCountInBody == 2) && (queringRule.op1IsAnd))
				{
					if (HazeProlog::GetVariableCountOfQuery(&queringRule.fact1) == 2) // exchange fact1 with fact2
					{
						Fact tmp;
						HazeProlog::CopyFact(&tmp, &queringRule.fact1);
						HazeProlog::CopyFact(&queringRule.fact1, &queringRule.fact2);
						HazeProlog::CopyFact(&queringRule.fact2, &tmp);
					}
				}

				int8 resultCountForFact1 = 0;
				bool hasResults = this->SolveQuery(&queringRule.fact1, &resultCountForFact1, fact1Results);

				PRINT_BUFFER_USAGE(resultCountForFact1, MAX_MATCHING_FACTS);

				if (queringRule.factCountInBody == 1)
				{
					for (int8 j = 0; j < resultCountForFact1; ++j) // add first fact results
					{
						HazeProlog::CopyFact(&results[(*resultCount)], &fact1Results[j]);
						++(*resultCount);
					}
				}
#ifndef NO_OR_RULES
				else if ((queringRule.factCountInBody == 2) && (!queringRule.op1IsAnd)) // OR with second fact
				{
					for (int8 j = 0; j < resultCountForFact1; ++j) // add first fact results
					{
						HazeProlog::CopyFact(&results[(*resultCount)], &fact1Results[j]);
						++(*resultCount);
					}

					int8 resultCountForFact2 = 0;
					bool hasResults2 = this->SolveQuery(&queringRule.fact2, &resultCountForFact2, fact1Results); // we can use same buffer(fact1Results) to store results.

					PRINT_BUFFER_USAGE(resultCountForFact2, MAX_MATCHING_FACTS);

					for (int8 j = 0; j < resultCountForFact2; ++j) // add second fact results
					{
						HazeProlog::CopyFact(&results[(*resultCount)], &fact1Results[j]);
						++(*resultCount);
					}

					hasResults |= hasResults2;
				}
#endif
				else if (hasResults && (queringRule.factCountInBody == 2) && (queringRule.op1IsAnd)) // AND with second fact
				{
					int8 fact1VariableCount = HazeProlog::GetVariableCountOfQuery(&queringRule.fact1);
					bool hasResults2 = false;

					for (int8 j = 0; j < resultCountForFact1; ++j) // check each fact1 results
					{
						Fact queringFact;
						HazeProlog::CopyFact(&queringFact, &queringRule.fact2);

						// replace queringFact variables with fact1 result
						if (queringRule.fact1.isTerm1Var)
						{
							if (HazeProlog::StringCompare(queringRule.fact1.term1Name, queringFact.term1Name))
							{
								queringFact.term1Name = fact1Results[j].term1Name;
								queringFact.isTerm1Var = false;
							}
							else if ((queringFact.termCount == 2) && HazeProlog::StringCompare(queringRule.fact1.term1Name, queringFact.term2Name))
							{
								queringFact.term2Name = fact1Results[j].term1Name;
								queringFact.isTerm2Var = false;
							}
						}

						if ((queringRule.fact1.termCount == 2) && queringRule.fact1.isTerm2Var)
						{
							if (HazeProlog::StringCompare(queringRule.fact1.term2Name, queringFact.term1Name))
							{
								queringFact.term1Name = fact1Results[j].term1Name;
								queringFact.isTerm1Var = false;
							}
							else if ((queringFact.termCount == 2) && HazeProlog::StringCompare(queringRule.fact1.term2Name, queringFact.term2Name))
							{
								queringFact.term2Name = fact1Results[j].term1Name;
								queringFact.isTerm2Var = false;
							}
						}

						// solve queringFact
						if (fact1VariableCount == 1) // first fact has one variable
						{
							if (HazeProlog::GetVariableCountOfQuery(&queringRule.head) == 1) // output require one column results
							{
								hasResults2 |= this->SolveQuery(&queringFact, resultCount, results);
							}
							else // output require 2 column results. So, we need to re-arrange results according to query. Ex: "query(X,Y)" and we have "rule(X,Y) = fact1(X) , fact2(X,Y)"
							{
#ifndef NO_ALL_VAR_QUERIES
								OptFunc1(&hasResults2, &queringFact, &queringRule, fact1Results, resultCount, results, j);
#endif
							}
						}
						else if (fact1VariableCount == 2) // both facts has 2 variables. Ex: "query(X,Y)" and we have "rule(X,Y) = fact1(X,Z) , fact2(Z,Y)"
						{
#ifndef NO_ALL_VAR_QUERIES
							OptFunc2(&hasResults2, &queringFact, &queringRule, fact1Results, resultCount, results, j);
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

		return false;
	}

	NO_INLINE bool SolveQuery(const Fact *query, int8 *resultCount, Fact *results)
	{
#ifdef PRINT_FREE_MEM
		PRINT("SolveQuery Free Mem: ");
		PRINT(freeMemory());
		PRINT("\n");
#endif

		if (!this->SolveRuleQuery(query, resultCount, results)) // do we have matching rules?
		{
			return this->SolveFactQuery(query, resultCount, results); // search in facts list if we don't have matching rules.
		}
		return true;
	}

	// if query has one var then print first term of result
	// if query has two vars then print both terms of result
	// if query has no vars then print true
	static void PrintResultAccordingToQuery(Fact *query, Fact *result)
	{
		int8 varCount = HazeProlog::GetVariableCountOfQuery(query);

		if (varCount == 0)
		{
			PRINT("true");
		}
		else if (varCount == 1)
		{
			PRINT(result->term1Name);
		}
		else if (varCount == 2)
		{
			PRINT(result->term1Name);
			PRINT(", ");
			PRINT(result->term2Name);
		}

		PRINT("\n");
	}

#ifdef ENABLE_SERIAL_PARSER

	static uint8_t ReadStringFromSerial(char *buffer)
	{
		uint8_t inIndex = 0;

		while (!Serial.available()){} // wait till we receive data

		while (Serial.available() > 0)
		{
			buffer[inIndex] = Serial.read();
			++inIndex;
		}

		buffer[inIndex] = 0; // null termination
		return inIndex;
	}

	// Serial Monitor Options: No line ending
	void EvalSerialInput()
	{
		char inData[40];
		uint8_t inLength = HazeProlog::ReadStringFromSerial(inData);

		if (inLength > 0)
		{
			char* parenthesStart = strchr(inData, '(');
			if (parenthesStart)
			{
				char* parenthesEnd = strchr(inData, ')');
				if (parenthesEnd)
				{
					char* comma = strchr(inData, ',');
					bool hasComma = (comma == 0) ? false : true;

					Fact query;
					query.termCount = hasComma ? 2 : 1;

					query.predicateName = inData;
					*parenthesStart = 0;

					query.term1Name = parenthesStart + 1;
					*parenthesEnd = 0;

					if (hasComma) // two terms
					{
						*comma = 0;
						query.term2Name = comma + 1;
					}

					query.nextFact = 0;

					HazeProlog::FixVariableFlags(&query);

					int8 resultCount = 0;
					Fact results[MAX_MATCHING_FACTS];

					if (this->SolveQuery(&query, &resultCount, results))
					{
						for (int8 i = 0; i < resultCount; ++i)
							HazeProlog::PrintResultAccordingToQuery(&query, &results[i]);
					}
					else
					{
						Serial.println("no results!");
					}

					return;
				}
			}
		}

		Serial.println("Invalid query!");
	}

#endif

};

#endif
