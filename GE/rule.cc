#include <GE/rule.h>
#include <iostream>

// ============================================================
// RULE CONSTRUCTOR
// ============================================================

Rule::Rule()
{
    length = 0;
}

// ============================================================
// ADD SYMBOL
// ============================================================

void Rule::addSymbol(Symbol *s)
{
    data.push_back(s);
    length++;
}

// ============================================================
// GET SYMBOL POSITION
// ============================================================

int Rule::getSymbolPos(string s)
{
    for (int i = 0; i < length; i++)
    {
        if (data[i]->getName() == s)
            return i;
    }

    return -1;
}

// ============================================================
// GET SYMBOL
// ============================================================

Symbol *Rule::getSymbol(int pos) const
{
    if (pos < 0 || pos >= length)
        return NULL;

    return data[pos];
}

// ============================================================
// SET SYMBOL
// ============================================================

void Rule::setSymbol(
    int pos,
    Symbol *s
    )
{
    if (pos < 0 || pos >= length)
        return;

    data[pos] = s;
}

// ============================================================
// GET RULE LENGTH
// ============================================================

int Rule::getLength() const
{
    return length;
}

// ============================================================
// ORIGINAL printRule()
//
// Existing QGenClass code can continue to use:
//
// printRule(genome,pos,redo)
//
// This simply calls the new trace-enabled implementation
// with trace == nullptr.
// ============================================================

string Rule::printRule(
    vector<int> genome,
    int &pos,
    int &redo
    )
{
    return printRule(
        genome,
        pos,
        redo,
        nullptr
        );
}

// ============================================================
// TRACE-ENABLED printRule()
//
// This version performs normal genotype -> phenotype mapping,
// but additionally records meaningful codon decisions.
//
// Examples:
//
// FUNCTION:
//     sin / cos / exp / log
//
// BINARYOP:
//     + / - / * / /
//
// BOOLOP:
//     < / > / <= / >= / == / !=
//
// XXLIST:
//     x1 / x2 / ...
//
// The trace is later used by targeted genotype local search.
// ============================================================

string Rule::printRule(
    vector<int> genome,
    int &pos,
    int &redo,
    vector<CodonTrace> *trace
    )
{
    string str = "";
    string str2 = "";

    Rule *r;

    for (int i = 0; i < length; i++)
    {
        Symbol *s = data[i];

        if (s == NULL)
            return "";

        // ====================================================
        // TERMINAL SYMBOL
        // ====================================================

        if (s->getTerminalStatus())
        {
            str = str + s->getName();
        }

        // ====================================================
        // NON-TERMINAL SYMBOL
        // ====================================================

        else
        {
            // ------------------------------------------------
            // Check genome boundary
            // ------------------------------------------------

            if (pos >= static_cast<int>(genome.size()))
            {
                /*
                 * Preserve the behaviour of the original
                 * QGenClass mapper.
                 *
                 * Note:
                 * The original source returned immediately
                 * here, so the statements after return were
                 * unreachable.
                 */
                return str;
            }

            // ------------------------------------------------
            // Non-terminal without productions
            // ------------------------------------------------

            if (
                s->getCountRules() == 0 &&
                !s->getTerminalStatus()
                )
            {
                return
                    "NONE" +
                    s->getName();
            }

            // ------------------------------------------------
            // CURRENT CODON INFORMATION
            // ------------------------------------------------

            int genomePos =
                pos;

            int ruleCount =
                s->getCountRules();

            int selectedRule =
                genome[pos] %
                ruleCount;

            // =================================================
            // RECORD TRACE
            // =================================================

            if (trace != nullptr)
            {
                CodonType type =
                    CodonType::UNKNOWN;

                string symbolName =
                    s->getName();

                if(symbolName == "FUNCTION")
                {
                    type =
                        CodonType::FUNCTION;
                }
                else if(symbolName == "BINARYOP")
                {
                    type =
                        CodonType::BINARY_OPERATOR;
                }
                else if(symbolName == "BOOLOP")
                {
                    type =
                        CodonType::BOOLEAN_OPERATOR;
                }
                else if(symbolName == "XXLIST")
                {
                    type =
                        CodonType::VARIABLE;
                }
                else if(
                    symbolName == "TERMINAL" ||
                    symbolName == "DIGITLIST" ||
                    symbolName == "DIGIT0"
                    )
                {
                    type =
                        CodonType::CONSTANT;
                }
                /*
                 * CLASS_OUTPUT is intentionally not added
                 * here yet.
                 *
                 * Class selection in QGenClass is handled at
                 * a higher level in ClassProgram, so we will
                 * trace that separately when we modify
                 * classprogram.cc.
                 */

                if (
                    type !=
                    CodonType::UNKNOWN
                    )
                {
                    trace->push_back(
                        CodonTrace(
                            genomePos,
                            ruleCount,
                            selectedRule,
                            type
                            )
                        );
                }
            }

            // =================================================
            // NORMAL RULE SELECTION
            // =================================================

            r =
                s->getRule(
                    selectedRule
                    );

            pos++;

            // ------------------------------------------------
            // Genome exhausted
            // ------------------------------------------------

            if (
                pos >=
                static_cast<int>(
                    genome.size()
                    )
                )
            {
                return str;
            }

            if (redo >= REDO_MAX)
                return str;

            // =================================================
            // RECURSIVE MAPPING
            //
            // IMPORTANT:
            //
            // Pass the SAME trace pointer so that all nested
            // grammar decisions are recorded.
            // =================================================

            str2 =
                r->printRule(
                    genome,
                    pos,
                    redo,
                    trace
                    );

            str =
                str +
                str2;
        }
    }

    return str;
}

// ============================================================
// GET VALUE
//
// This part is kept functionally identical to the original
// QGenClass implementation.
// ============================================================

double Rule::getValue(
    vector<int> genome,
    int &pos,
    int &redo,
    DoubleStack &stack,
    double *X
    )
{
    for (int i = 0; i < length; i++)
    {
        Symbol *s =
            data[i];

        if (s->getTerminalStatus())
        {
            string str =
                s->getName();

            if (
                str ==
                string("+")
                )
            {
                double a, b;

                a =
                    stack.pop();

                b =
                    stack.pop();

                cout
                    << "a ="
                    << a
                    << "b="
                    << b
                    << endl;

                stack.push(
                    b + a
                    );
            }

            else if (
                str ==
                "-"
                )
            {
                double a, b;

                a =
                    stack.pop();

                b =
                    stack.pop();

                stack.push(
                    b - a
                    );
            }

            else if (
                str ==
                "*"
                )
            {
                double a, b;

                a =
                    stack.pop();

                b =
                    stack.pop();

                stack.push(
                    b * a
                    );
            }

            else if (
                str ==
                "x"
                )
            {
                stack.push(
                    X[0]
                    );
            }
        }

        else
        {
            if (
                pos >=
                static_cast<int>(
                    genome.size()
                    )
                )
            {
                redo++;
                pos = 0;
            }

            if (
                s->getCountRules() ==
                0
                )
            {
                return 0.0;
            }

            int k =
                genome[pos] %
                s->getCountRules();

            /*
             * k is intentionally retained because it exists
             * in the original implementation and may be
             * useful when debugging.
             */
            (void)k;

            Rule *r;

            r =
                s->getRule(
                    genome[pos] %
                    s->getCountRules()
                    );

            pos++;

            if (
                pos >=
                static_cast<int>(
                    genome.size()
                    )
                )
            {
                redo++;
                pos = 0;
            }

            if (redo >= REDO_MAX)
                return 0;

            return
                r->getValue(
                    genome,
                    pos,
                    redo,
                    stack,
                    X
                    );
        }
    }

    return stack.top();
}

// ============================================================
// DESTRUCTOR
// ============================================================

Rule::~Rule()
{
}
