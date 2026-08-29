#ifndef __RULE__H
#define __RULE__H

#include <GE/symbol.h>
#include <GE/doublestack.h>

#include <string>
#include <vector>

#define REDO_MAX 1

using namespace std;

// ============================================================
// TARGETED GENOTYPE MUTATION SUPPORT
// ============================================================

/**
 * @brief Describes the semantic meaning of a codon used
 *        during genotype-to-phenotype mapping.
 *
 * This information will later be used by the targeted
 * genotype local search.
 */
enum class CodonType
{
    UNKNOWN,

    VARIABLE,

    BINARY_OPERATOR,

    FUNCTION,

    BOOLEAN_OPERATOR,

    // Codons participating only in the generation
    // of numerical constants.
    CONSTANT,

    CLASS_OUTPUT
};
/**
 * @brief Stores information about a codon that participated
 *        in a meaningful grammar decision.
 *
 * Example:
 *
 * genome[17] = 13
 *
 * Suppose the corresponding symbol has four rules:
 *
 * 0 -> sin
 * 1 -> cos
 * 2 -> exp
 * 3 -> log
 *
 * Since:
 *
 * 13 % 4 = 1
 *
 * the selected production is "cos".
 *
 * This structure stores:
 *
 * genomePos   = 17
 * ruleCount   = 4
 * selectedRule = 1
 * type        = FUNCTION
 */
struct CodonTrace
{
    /**
     * Position of the codon in the genome used by this
     * mapping operation.
     */
    int genomePos;

    /**
     * Number of alternative production rules available
     * for the corresponding grammar symbol.
     */
    int ruleCount;

    /**
     * Selected production:
     *
     * genome[genomePos] % ruleCount
     */
    int selectedRule;

    /**
     * Semantic category of this grammar decision.
     */
    CodonType type;

    CodonTrace()
    {
        genomePos = -1;
        ruleCount = 0;
        selectedRule = -1;
        type = CodonType::UNKNOWN;
    }

    CodonTrace(
        int genomePos_,
        int ruleCount_,
        int selectedRule_,
        CodonType type_
        )
    {
        genomePos = genomePos_;
        ruleCount = ruleCount_;
        selectedRule = selectedRule_;
        type = type_;
    }
};

// ============================================================
// RULE
// ============================================================

/**
 * @brief The Rule class creates grammar production rules
 *        and evaluates them using an integer genome.
 */
class Rule
{
private:

    /**
     * Symbols that constitute this production rule.
     */
    vector<Symbol*> data;

    /**
     * Number of symbols in this rule.
     */
    int length;

public:

    /**
     * @brief Main constructor.
     */
    Rule();

    /**
     * @brief Adds a symbol to the rule.
     *
     * @param s Symbol to add.
     */
    void addSymbol(Symbol *s);

    /**
     * @brief Searches for a symbol by name.
     *
     * @param s Symbol name.
     *
     * @return Position of the symbol or an appropriate
     *         failure value according to the implementation.
     */
    int getSymbolPos(string s);

    /**
     * @brief Returns the symbol at a specified position.
     *
     * @param pos Position.
     *
     * @return Pointer to Symbol.
     */
    Symbol *getSymbol(int pos) const;

    /**
     * @brief Replaces a symbol at a specified position.
     *
     * @param pos Position.
     * @param s New symbol.
     */
    void setSymbol(
        int pos,
        Symbol *s
        );

    /**
     * @brief Returns the number of symbols in the rule.
     *
     * @return Rule length.
     */
    int getLength() const;

    // ========================================================
    // ORIGINAL MAPPING INTERFACE
    // ========================================================

    /**
     * @brief Maps an integer chromosome to a textual rule.
     *
     * This is the original interface used by QGenClass.
     * It remains available so existing code does not need
     * to be changed immediately.
     *
     * Internally, the implementation in rule.cc will call
     * the trace-enabled version with trace == nullptr.
     *
     * @param genome Integer genome.
     * @param pos Current genome position.
     * @param redo Number of genome wraps.
     *
     * @return Generated rule as a string.
     */
    string printRule(
        vector<int> genome,
        int &pos,
        int &redo
        );

    // ========================================================
    // NEW TRACE-ENABLED MAPPING INTERFACE
    // ========================================================

    /**
     * @brief Maps an integer chromosome to a textual rule and
     *        optionally records meaningful codon decisions.
     *
     * If trace is not nullptr, every important grammar choice
     * can be recorded as a CodonTrace entry.
     *
     * This information allows later targeted mutations such as:
     *
     *   cos -> sin
     *   sin -> exp
     *   +   -> *
     *   -   -> /
     *   x2  -> x7
     *   <   -> >=
     *
     * by modifying the exact chromosome codon that generated
     * that part of the phenotype.
     *
     * @param genome Integer genome.
     * @param pos Current genome position.
     * @param redo Number of genome wraps.
     * @param trace Optional output vector for codon trace.
     *
     * @return Generated rule as a string.
     */
    string printRule(
        vector<int> genome,
        int &pos,
        int &redo,
        vector<CodonTrace> *trace
        );

    // ========================================================
    // DIRECT RULE EVALUATION
    // ========================================================

    /**
     * @brief Evaluates this rule directly.
     *
     * This method is kept unchanged from the original
     * QGenClass interface.
     *
     * @param genome Integer genome.
     * @param pos Current genome position.
     * @param redo Number of genome wraps.
     * @param stack Evaluation stack.
     * @param X Input feature vector.
     *
     * @return Numerical result.
     */
    double getValue(
        vector<int> genome,
        int &pos,
        int &redo,
        DoubleStack &stack,
        double *X
        );

    /**
     * @brief Destructor.
     */
    ~Rule();
};

#endif
