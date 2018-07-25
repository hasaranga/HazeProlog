
//#define MONITOR_BUFFERS

#include <stdio.h>
#include "../HazeProlog.h"

static const Fact fact12{ 2, "understands", false, "ann", false, "tom", 0 };
static const Fact fact11{ 2, "understands", false, "madona", false, "tom", &fact12 };
static const Fact fact10{ 1, "female", false, "madona", false, "", &fact11 };
static const Fact fact9{ 1, "female", false, "ann", false, "", &fact10 };
static const Fact fact8{ 2, "likes", false, "madona", false, "wine", &fact9 };
static const Fact fact7{ 2, "likes", false, "ann", false, "wine", &fact8 };
static const Fact fact6{ 2, "likes", false, "john", false, "wine", &fact7 };
static const Fact fact5{ 1, "fruit", false, "apple", false, "", &fact6 };
static const Fact fact4{ 2, "fatherOf", false, "tom", false, "dick", &fact5 };
static const Fact fact3{ 2, "motherOf", false, "dick", false, "jane", &fact4 };
static const Fact fact2{ 2, "motherOf", false, "ann", false, "marry", &fact3 };
static const Fact fact1{ 2, "motherOf", false, "marry", false, "judy", &fact2 };

Rule rule7{ { 2, "female-with-like-to", true, "X", true, "Y", 0 }, 2
, { 2, "likes", true, "X", true, "Y", 0 }, true
, { 1, "female", true, "X", false, "", 0 }, 0 };

Rule rule6{ { 2, "friend-with", false, "tom", true, "X", 0 }, 1
, { 2, "understands", true, "X", false, "tom", 0 }, false
, { 0, "", false, "", false, "", 0 }, &rule7 };

Rule rule5{ { 1, "is-bitch", true, "X", false, "", 0 }, 2
, { 1, "female", true, "X", false, "", 0 }, true
, { 2, "likes", true, "X", false, "wine", 0 }, &rule6 };

Rule rule4{ { 1, "john-likes-mother", true, "X", false, "", 0 }, 2
, { 1, "john-likes", true, "X", false, "", 0 }, true
, { 2, "motherOf", true, "X", false, "marry", 0 }, &rule5 };

Rule rule3{ { 2, "likes", false, "john", true, "X", 0 }, 1
, { 2, "likes", true, "X", false, "wine", 0 }, false
, { 0, "", true, "", false, "", 0 }, &rule4 };

Rule rule2{ { 2, "grandMotherOf", true, "X", true, "GM", 0 }, 2
, { 2, "fatherOf", true, "X", true, "F", 0 }, true
, { 2, "motherOf", true, "F", true, "GM", 0 }, &rule3 };

Rule rule1{ { 2, "grandMotherOf", true, "X", true, "GM", 0 }, 2
, { 2, "motherOf", true, "X", true, "F", 0 }, true
, { 2, "motherOf", true, "F", true, "GM", 0 }, &rule2 };


int main()
{
	// uncomment to test each query

	//Fact query{ 2, "female-with-like-to", true, "Female", true, "Like", 0 };
	//Fact query{ 2, "likes", false, "john", true, "X", 0 };
	Fact query{ 2, "grandMotherOf", true, "X", true, "GM", 0 };
	//Fact query{ 2, "motherOf", true, "X", true, "jane", 0 };
	//Fact query{ 1, "fruit", true, "X", false, "", 0 };

	//FixVariableFlags(&query); // no need to call if you are manualy set variable flags!

	int resultCount = 0;
	Fact results[MAX_MATCHING_FACTS];

	if (SolveQuery(&query, &rule1, &fact1, &resultCount, results))
	{
		for (int i = 0; i < resultCount; ++i)
			PrintResultAccordingToQuery(&query, &results[i]);
	}
	else
	{
		printf("no results!\n");
	}

	return 0;
}