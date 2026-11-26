// Script parser buffers (ListBank = ResourceBanks = TokenReader: one object). It holds 8 groups of up to 0x80
// named entries (text, value, flag), the script text being parsed and an error flag (m_3632c) that makes every
// reader fail once an error was reported.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "listbank.h"
#include "script_statements.h"
// Clears group `group`: entries that are not in use lose their values. Returns 1.
int ListBank::M_10004680(int group)
{
    variableCounts[group] = 0;
    for (int i = 0; i < 0x80; i++)
    {
        if (borrowed[group][i] == 0)
        {
            values[group][i].storage = 0;
            borrowed[group][i] = 0;
        }
    }
    return 1;
}

// Appends entry (`text`, `value`, `flag`) to group `group` and marks it in use. Returns 1.
int ListBank::M_100046d0(const char *text, int group, int value, char flag)
{
    borrowed[group][variableCounts[group]] = 1;
    strcpy(variableNames[group][variableCounts[group]], text);
    values[group][variableCounts[group]].value = value;
    variableTypes[group][variableCounts[group]] = flag;
    variableCounts[group]++;
    return 1;
}

// Looks up `name` among the 16 registered keywords (prefix match); stores its index and returns 1, else 0.
int ListBank::M_100048f0(const char *name, int *index)
{
    for (int i = 0; i < 0x10; i++)
    {
        if (m_36334[i][0] != 0 && strncmp(name, m_36334[i], strlen(m_36334[i])) == 0)
        {
            *index = i;
            return 1;
        }
    }
    return 0;
}

// Formats a script value into `buffer` (256 bytes): type 0 int (%d), 1 float (%f), 2 string. Returns 1.
int ListBank::M_10004950(char *buffer, void *value, char type)
{
    if (type == 0)
        sprintf(buffer, "%d", *(int *)value);
    else if (type == 1)
        sprintf(buffer, "%f", *(float *)value);
    else if (type == 2)
        sprintf(buffer, "%s", (char *)value);
    buffer[0xff] = 0;
    return 1;
}

// 1 when `text` contains a parenthesis (a function call), 0 otherwise or after an error.
int ListBank::M_100049e0(const char *text)
{
    int length = strlen(text);
    for (int i = 0; i < length; i++)
    {
        if (failed != 0)
            return 0;
        if (text[i] == '(' || text[i] == ')')
            return 1;
    }
    return 0;
}

// Parses `text` as an integer when it only holds digits and signs; returns 1 on success.
int ListBank::M_10004a30(const char *text, int *value)
{
    int length = strlen(text);
    for (int i = 0; i < length; i++)
    {
        if (failed != 0)
            return 0;
        if (text[i] != '+' && text[i] != '-' && (text[i] < '0' || text[i] > '9'))
            return 0;
    }
    *value = atol(text);
    return 1;
}

// Parses `text` as a float when it only holds digits, signs and a decimal point; returns 1 on success.
int ListBank::M_10004aa0(const char *text, float *value)
{
    int length = strlen(text);
    for (int i = 0; i < length; i++)
    {
        if (failed != 0)
            return 0;
        if (text[i] != '+' && text[i] != '-' && text[i] != '.' && (text[i] < '0' || text[i] > '9'))
            return 0;
    }
    *value = (float)atof(text);
    return 1;
}

// Reports a script parse error: sets the error flag (m_3632c, which makes the readers above fail from now on),
// prints the message, passes it and the error's offset in the script text to the host callback, and frees all
// script buffers (ScriptBuffers_FreeAll 0x10007d50). Returns 0.
int ListBank::ReportError(const char *message)
{
    failed = 1;
    printf("%s\n", message);
    if (callback != 0)
        callback(1, (int)message);
    if (callback != 0)
        callback(5000, cursor[depth] - text);
    FreeAll();
    return 0;
}

// Collects the index of every declared name equal to `name` (case-sensitive) into `out` and returns how many were
// found; stops with 0 once a parse error was reported.
int ListBank::FindAll(const char *name, int *out)
{
    int found = 0;
    for (int i = 0; i < builtinCount; i++)
    {
        if (failed != 0)
            return 0;
        if (strcmp(name, builtinNames[i]) == 0)
            out[found++] = i;
    }
    return found;
}

// Compiles the whole script text: top-level statements are compiled one by one, and each `{ ... }` block is recorded
// in `blocks` and skipped (blocks are compiled when they run). Script variables start again from the global ones.
// Returns 1, or 0 when an error stopped the compile.
int ListBank::CompileText()
{
    char *p;
    int level;
    char statement[256];
    FreeBank(0);
    int savedDepth = depth;
    builtinCount = globalBuiltinCount;
    compiling = 1;
    blockCount = 0;
    p = text;
    depth = 0;
    while (p < text + textLength)
    {
        if (failed != 0)
            return 0;
        ReadStatement(&p, statement);
        cursor[0] = p;
        if (statement[0] == 0)
            p++;
        else if (statement[0] == '{')
        {
            level = 1;
            blocks[blockCount].start = p;
            SkipBlock(&p, &level, blockCount);
            blockCount++;
        }
        else
            CompileStatement(statement);
    }
    depth = savedDepth;
    compiling = 0;
    return 1;
}

// 0x10035a30. Skips a `{ ... }` block whose opening brace was just read: nested blocks are recorded in `blocks` (and their
// ends), braces inside double quotes do not count. On the closing brace *text points past it, *level is lowered and
// the end of block `block` is recorded; returns 1. Reaching the end of the text reports "}が足りない。"
// ("missing }") and returns 0.
int ListBank::SkipBlock(char **text, int *level, int block)
{
    int inQuotes = 0;
    char *p = *text;
    while (p < this->text + textLength)
    {
        if (failed != 0)
            return 0;
        if (*p == '"')
        {
            if (inQuotes == 1)
                inQuotes = 0;
            else if (inQuotes == 0)
                inQuotes = 1;
        }
        if (inQuotes == 0)
        {
            if (*p == '{')
            {
                (*level)++;
                p++;
                *text = p;
                blockCount++;
                blocks[block].start = p;
                SkipBlock(text, level, blockCount);
                p = *text;
            }
            if (*p == '}')
            {
                p++;
                blocks[block].end = p;
                (*level)--;
                *text = p;
                return 1;
            }
        }
        p++;
    }
    ReportError("}\x82\xaa\x91\xab\x82\xe8\x82\xc8\x82\xa2\x81" "B");
    *text = p;
    return 0;
}

// 0x10035b10. Reads the next statement into `statement` (at most 255 characters, followed by two zero bytes). A
// '{', '}' or ';' at the cursor is a statement of its own. Otherwise separators are skipped and the statement runs to a
// ';', brace or the end of the text outside quotes, brackets and parentheses (the ';' is consumed, a brace is left
// for the next call), or up to and including a ':' (a label). A closing parenthesis of the enclosing call ends it
// too, and after a ')' a letter or '_' (past any white space) starts the next statement. Tabs and line ends outside
// quotes are dropped. Returns 1, or 0 at the end of the text or after an error.
int ListBank::ReadStatement(char **text, char *statement)
{
    char *p = *text;
    statement[0] = 0;
    int inQuotes = 0;
    int parens = 0;
    int brackets = 0;
    if (*p == 0)
        return 0;
    if (*p == '{' || *p == '}' || *p == ';')
    {
        statement[0] = *p;
        statement[1] = 0;
        *text = p + 1;
        return 1;
    }
    while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    int afterParen = 0;
    int length = 0;
    do
    {
        if (failed != 0)
            return 0;
        if (inQuotes == 0)
        {
            while (*p == '\t' || *p == '\n' || *p == '\r')
                p++;
        }
        if (afterParen && ((*p >= 'a' && *p <= 'z') || *p == '_' || (*p >= 'A' && *p <= 'Z')))
        {
            *text = p;
            statement[length] = 0;
            statement[length + 1] = 0;
            return 1;
        }
        if (*p == '"')
        {
            if (inQuotes == 0)
                inQuotes = 1;
            else if (inQuotes == 1)
                inQuotes = 0;
        }
        if (*p != ' ' && *p != '\n' && *p != '\r' && *p != '\t')
            afterParen = 0;
        if (inQuotes == 0)
        {
            if (*p == '(')
                parens++;
            else if (*p == ')')
            {
                parens--;
                afterParen = 1;
            }
            if (parens < 0)
            {
                *text = p + 1;
                return 1;
            }
            if (*p == '[')
                brackets++;
            else if (*p == ']')
                brackets--;
            if (parens == 0 && brackets == 0 && (*p == ':' || *p == 0 || *p == ';' || *p == '{' || *p == '}'))
            {
                if (*p == ':')
                {
                    statement[length] = *p;
                    length++;
                    p++;
                    *text = p;
                    statement[length] = 0;
                    statement[length + 1] = 0;
                    return 1;
                }
                if (*p != '{' && *p != '}' && *p != ';')
                    p++;
                *text = p;
                statement[length] = 0;
                statement[length + 1] = 0;
                return 1;
            }
        }
        statement[length] = *p;
        length++;
        p++;
        statement[length] = 0;
        statement[length + 1] = 0;
    } while (length < 0x100);
    ReportError("\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B");
    *text = p;
    return 1;
}

// 0x10035d20. Reads the next argument of a call into `word`, like ReadWord, but parentheses nest too: the argument
// ends at a space, comma, ';' or the end of the text outside quotes, brackets and parentheses, or at the closing
// parenthesis of the call (not copied). *text is moved past the character that ended it.
int ListBank::ReadArgument(char **text, char *word)
{
    char *p = *text;
    word[0] = 0;
    int length = 0;
    int inQuotes = 0;
    int parens = 0;
    int brackets = 0;
    if (*p == 0)
        return 0;
    while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ';')
        p++;
    do
    {
        if (failed != 0)
            return 0;
        if (*p == '"')
        {
            if (inQuotes == 0)
                inQuotes = 1;
            else if (inQuotes == 1)
                inQuotes = 0;
        }
        if (inQuotes == 0)
        {
            if (*p == '(')
                parens++;
            else if (*p == ')')
                parens--;
            if (parens < 0)
            {
                *text = p + 1;
                return 1;
            }
            if (*p == '[')
                brackets++;
            else if (*p == ']')
                brackets--;
            if (parens == 0 && brackets == 0 && (*p == 0 || *p == ' ' || *p == ',' || *p == ';'))
            {
                *text = p + 1;
                return 1;
            }
        }
        word[length] = *p;
        length++;
        p++;
        word[length] = 0;
        word[length + 1] = 0;
    } while (length < 0x100);
    ReportError("\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B");
    *text = p;
    return 1;
}


// 0x10035e80. Reads the next word of a statement into `word` (at most 255 characters, followed by two zero bytes):
// separators (space, comma, tab, line ends, ';') before it are skipped, and the word ends at a separator, a
// parenthesis or the end of the text outside double quotes and outside `[ ]` brackets. *text is moved past the word
// and its separator (a parenthesis or the end of the text is left in place). Returns 1, or 0 at the end of the text
// or after an error. A word that is too long reports "型が異常です。" ("the type is invalid") and returns 1.
int ListBank::ReadWord(char **text, char *word)
{
    char *p = *text;
    word[0] = 0;
    int length = 0;
    int inQuotes = 0;
    int brackets = 0;
    if (*p == 0)
        return 0;
    while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ';')
        p++;
    do
    {
        if (failed != 0)
            return 0;
        if (*p == '"')
        {
            if (inQuotes == 0)
                inQuotes = 1;
            else if (inQuotes == 1)
                inQuotes = 0;
        }
        if (inQuotes == 0)
        {
            if (*p == '[')
                brackets++;
            else if (*p == ']')
                brackets--;
            if (brackets == 0 && (*p == '\n' || *p == '\r' || *p == 0 || *p == ')' || *p == '(' || *p == ' ' ||
                                  *p == ',' || *p == ';'))
            {
                if (*p != 0 && *p != ')' && *p != '(')
                    p++;
                *text = p;
                return 1;
            }
        }
        word[length] = *p;
        length++;
        p++;
        word[length] = 0;
        word[length + 1] = 0;
    } while (length < 0x100);
    ReportError("\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B");
    *text = p;
    return 1;
}

// 0x10035fa0. Like ReadWord, but '&' also ends a word, and a '&' at the start of a word is read as a word of its
// own (so `a&b` reads as "a", "&", "b").
int ListBank::ReadTerm(char **text, char *word)
{
    char *p = *text;
    word[0] = 0;
    int length = 0;
    int inQuotes = 0;
    int brackets = 0;
    if (*p == 0)
        return 0;
    while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ';')
        p++;
    do
    {
        if (failed != 0)
            return 0;
        if (*p == '"')
        {
            if (inQuotes == 0)
                inQuotes = 1;
            else if (inQuotes == 1)
                inQuotes = 0;
        }
        if (inQuotes == 0)
        {
            if (*p == '[')
                brackets++;
            else if (*p == ']')
                brackets--;
            if (brackets == 0 && (*p == '&' || *p == '\n' || *p == '\r' || *p == 0 || *p == ')' || *p == '(' ||
                                  *p == ' ' || *p == ',' || *p == ';'))
            {
                if (*p == '&' && length == 0)
                {
                    word[0] = *p;
                    word[1] = 0;
                    word[2] = 0;
                    *text = p + 1;
                    return 1;
                }
                if (*p != '&' && *p != 0 && *p != ')' && *p != '(')
                    p++;
                *text = p;
                return 1;
            }
        }
        word[length] = *p;
        length++;
        p++;
        word[length] = 0;
        word[length + 1] = 0;
    } while (length < 0x100);
    ReportError("\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B");
    *text = p;
    return 1;
}

// 0x100360f0. Reads the next token of an expression into `token` (at most 255 characters, followed by two zero
// bytes): an operand (a name, number or quoted text, with its `( )` and `[ ]` parts) or an operator. Separators
// before it are skipped. An operand ends at an operator, a space, ';' or the end of the text outside quotes and
// brackets; an operator is one character, or two for `&& || -- ++ /= *= += -= %= != <= >= ==`. token[0xff] is set to
// 1 when the token is an operator. A '(' or ')' right after an operator ends the operator. Returns 1, or 0 at the
// end of the text or after an error.
int ListBank::ReadToken(char **text, char *token)
{
    char *p = *text;
    int length = 0;
    token[TOKEN_IS_OPERATOR] = 0;
    token[0] = 0;
    int inQuotes = 0;
    int brackets = 0;
    int parens = 0;
    int last = TOKEN_START;
    if (*p == 0)
        return 0;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ';')
        p++;
    do
    {
        if (failed != 0)
            return 0;
        if (*p == '"')
        {
            if (inQuotes == 0)
                inQuotes = 1;
            else if (inQuotes == 1)
                inQuotes = 0;
        }
        if (inQuotes == 0)
        {
            while (*p == ' ' || *p == '\t')
                p++;
            if (*p == '(')
            {
                if (last == TOKEN_OPERATOR)
                {
                    *text = p;
                    return 1;
                }
                parens++;
            }
            else if (*p == ')')
            {
                if (last == TOKEN_OPERATOR)
                {
                    *text = p;
                    return 1;
                }
                parens--;
            }
            if (*p == '[')
                brackets++;
            else if (*p == ']')
                brackets--;
            if (parens == 0 && brackets == 0)
            {
                if (*p == ',' || *p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '=' || *p == '%' ||
                    *p == '<' || *p == '>' || *p == '&' || *p == '!' || *p == '|')
                {
                    // an operator: it ends the operand before it
                    if (last == TOKEN_OPERAND)
                    {
                        *text = p;
                        return 1;
                    }
                    token[length] = *p;
                    token[length + 1] = 0;
                    p++;
                    last = TOKEN_OPERATOR;
                    // 1 = the next character cannot continue the operator
                    int ends = 1;
                    if (*p == ',' || *p == '+' || *p == '*' || *p == '/' || *p == '=' || *p == '%' || *p == '<' ||
                        *p == '>' || *p == '&' || *p == '!' || *p == '|')
                        ends = 0;
                    token[TOKEN_IS_OPERATOR] = 1;
                    char op = token[length];
                    // a two-character operator (each pair is tested on its own)
                    if (op == '&' && *p == '&')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '|' && *p == '|')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '-' && *p == '-')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '+' && *p == '+')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '/' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '*' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '+' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '-' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '%' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '!' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '<' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '>' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    if (op == '=' && *p == '=')
                    {
                        length++;
                        token[length] = *p;
                        token[length + 1] = 0;
                        *text = p + 1;
                        return 1;
                    }
                    // a sign or negation after the operator starts the next operand
                    if (*p == '!' || *p == '+' || *p == '-')
                        ends = 1;
                    // a single-character operator
                    if (op == '*' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '/' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '%' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '<' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '>' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '!' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '=' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '-' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == '+' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                    if (op == ',' && ends)
                    {
                        *text = p;
                        return 1;
                    }
                }
                else
                {
                    // an operand character: it ends the operator before it
                    if (last == TOKEN_OPERATOR)
                    {
                        *text = p;
                        return 1;
                    }
                    last = TOKEN_OPERAND;
                }
                if (*p == 0 || *p == ' ' || *p == ';')
                {
                    if (*p == 0)
                        p[1] = 0;
                    *text = p + 1;
                    return 1;
                }
            }
        }
        token[length] = *p;
        length++;
        p++;
        token[length] = 0;
        token[length + 1] = 0;
    } while (length < 0x100);
    ReportError("\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B");
    *text = p;
    return 1;
}

// 0x10036560. Copies the `n`-th (from 0) bracketed part of `name` into `part`: for "a[1][x+2]", part 1 is "x+2"
// (inner brackets and quoted text are kept, at most 255 characters, followed by two zero bytes). Returns 1 once the
// part is complete, 0 when `name` ends first (at its end, a space, comma or ';' outside the brackets), at an unmatched
// ')' or after an error.
int ListBank::GetIndexPart(const char *name, char *part, int n)
{
    part[0] = 0;
    int length = 0;
    int inQuotes = 0;
    int brackets = 0;
    int seenBracket = 0;
    int index = 0;
    int parens = 0;
    if (*name == 0)
        return 0;
    while (*name == ' ' || *name == ',' || *name == '\t' || *name == '\r' || *name == '\n' || *name == ';')
        name++;
    do
    {
        if (failed != 0)
            return 0;
        if (*name == 0)
            return 0;
        if (*name == '"')
        {
            if (inQuotes == 0)
                inQuotes = 1;
            else if (inQuotes == 1)
                inQuotes = 0;
        }
        if (inQuotes == 0)
        {
            if (*name == '(')
                parens++;
            else if (*name == ')')
                parens--;
            if (parens < 0)
                return 0;
            if (*name == '[')
            {
                brackets++;
                seenBracket = 1;
                if (brackets == 1)
                    name++;
            }
            else if (*name == ']')
                brackets--;
            if (brackets == 0 && seenBracket == 1)
            {
                if (n <= index)
                    return 1;
                length = 0;
                part[0] = 0;
                part[1] = 0;
                index++;
            }
            if (parens == 0 && brackets == 0)
            {
                if (*name == 0 || *name == ' ' || *name == ',' || *name == ';')
                    return 0;
                name++;
                continue;
            }
        }
        if (brackets >= 1)
        {
            part[length] = *name;
            length++;
            part[length] = 0;
            part[length + 1] = 0;
        }
        name++;
    } while (length < 0x100);
    ReportError("\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B");
    return 0;
}

// 0x100fb6d0. Copies the script `source` (`length` bytes, plus the byte after them) into `text` and compiles it.
// "//" comments outside double quotes are blanked out on the way: from the second slash to the end of the line
// every character becomes '\r', and the first slash is dropped. Returns 1.
int ListBank::LoadText(const char *source, int length)
{
    int inQuotes = 0;
    int inComment = 0;
    int out = 0;
    self = this;
    textLength = 0;
    memset(text, 0, sizeof(text));
    for (int in = 0; in <= length;)
    {
        char c = source[in];
        in++;
        text[out] = c;
        if (inComment == 0)
        {
            if (c == '"' && inQuotes == 0)
                inQuotes = 1;
            else if (c == '"' && inQuotes == 1)
                inQuotes = 0;
            if (out > 0 && inQuotes == 0 && c == '/' && text[out - 1] == '/')
            {
                // Start of a comment: drop the first slash and blank out the rest of the line.
                out--;
                textLength--;
                inComment = 1;
            }
        }
        if (inComment == 1)
        {
            if (text[out] == '\r' || text[out] == '\n')
            {
                inComment = 0;
            }
            else
            {
                text[out] = '\r';
                out--;
                textLength--;
            }
        }
        textLength++;
        out++;
    }
    CompileText();
    return 1;
}

// 0x10037470. For an array reference such as "name[expr]..." (a '[' after the first character): evaluates every
// bracketed index expression in turn and stores the value of the first one in `value` (0 when there is no '[').
// Up to eight index values are kept, though only the first is used. Returns 0.
int ListBank::FirstIndexValue(const char *name, int *value)
{
    int values[8];
    char part[256];
    *value = 0;
    int bracket = 0;
    for (int i = (int)strlen(name) - 1; i > 0; i--)
    {
        if (name[i] == '[')
            bracket = 1;
    }
    char copy[256];
    if (bracket != 0)
    {
        int n = 0;
        strcpy(copy, name);
        while (GetIndexPart(copy, part, n) != 0)
        {
            Evaluate(part, 0);
            values[n] = *result;
            n++;
        }
        *value = values[0];
    }
    return 0;
}

// 0x10004760. Declares variable `name` as the next entry of group `group` with storage of its own: types 0 and 1
// (numbers) get a new int set to 0, type 2 (text) a new 256-byte string set to "NULL". The entry is marked as not
// in use (m_e80c = 0) and keeps a pointer to the storage in m_440c. Returns 1.
// The type 0 and type 1 branches are identical; cl merges them with a jump back (as in the original).
int ListBank::DeclareVariable(const char *name, int group, char type)
{
    borrowed[group][variableCounts[group]] = 0;
    strcpy(variableNames[group][variableCounts[group]], name);
    if (type == 0)
    {
        values[group][variableCounts[group]].ints = new int;
        *values[group][variableCounts[group]].ints = 0;
    }
    else if (type == 1)
    {
        values[group][variableCounts[group]].ints = new int;
        *values[group][variableCounts[group]].ints = 0;
    }
    else if (type == 2)
    {
        values[group][variableCounts[group]].texts = new char[1][LISTBANK_TEXT_SIZE];
        strcpy(values[group][variableCounts[group]].texts[0], "NULL");
    }
    variableTypes[group][variableCounts[group]] = type;
    variableCounts[group]++;
    return 1;
}

// 0x100caf90. Compiles `expression` (its value lands in *result, of type resultType) and converts the value to
// `type`: 0 = int (a float is truncated, text is read with atol), 1 = float (text with atof), 2 = quoted text
// ("%d" or "%f" in quotes). An unknown result type gives 0, or "NULL" in quotes for text. Returns 1.
int ListBank::Evaluate(const char *expression, char type)
{
    CompileCall(expression, 0);
    if (type == 0)
    {
        if (resultType == 0)
        {
            // already an int
        }
        else if (resultType == 1)
            *result = (int)*(float *)result;
        else if (resultType == 2)
            *result = atol((char *)result + 1);
        else
            *result = 0;
    }
    else if (type == 1)
    {
        if (resultType == 0)
            *(float *)result = (float)*result;
        else if (resultType == 1)
        {
            // already a float
        }
        else if (resultType == 2)
            *(float *)result = (float)atof((char *)result + 1);
        else
            *result = 0;
    }
    else if (type == 2)
    {
        if (resultType == 0)
            sprintf((char *)result, "\"%d\"", *result);
        else if (resultType == 1)
            sprintf((char *)result, "\"%f\"", *(float *)result);
        else if (resultType != 2)
            strcpy((char *)result, "\"NULL\"");
    }
    return 1;
}

extern "C" void FUN_10005a70(int id, ScriptArgument *args, ListBank *bank);   // Script_LibraryBuiltin: runs the library built-ins

// 0x10037850 (Script_InitLibrary). Resets the parser (no text, no blocks, no names, no keywords, empty groups;
// results go to the scratch value at 0x2240c) and registers the library built-ins sin ... strcmp (ids 0x2774 to
// 0x278c). The names declared so far count as global ones. Returns 0.
int ListBank::InitLibrary()
{
    result = resultBuffer;
    self = this;
    callback = 0;
    flow = 0;
    blockCount = 0;
    compiling = 0;
    textLength = 0;
    depth = 0;
    failed = 0;
    builtinCount = 0;
    for (int i = 0; i < 0x20; i++)
        m_36334[i][0] = 0;
    for (int group = 0; group < 8; group++)
    {
        variableCounts[group] = 0;
        for (int k = 0; k < 0x80; k++)
            values[group][k].storage = 0;
    }
    RegisterBuiltin("float sin(float)", 0x2774, FUN_10005a70);
    RegisterBuiltin("float cos(float)", 0x2775, FUN_10005a70);
    RegisterBuiltin("float tan(float)", 0x2776, FUN_10005a70);
    RegisterBuiltin("float atan(float)", 0x2777, FUN_10005a70);
    RegisterBuiltin("float atan2(float,float)", 0x2778, FUN_10005a70);
    RegisterBuiltin("float sqrt(float)", 0x2779, FUN_10005a70);
    RegisterBuiltin("int rand()", 0x277a, FUN_10005a70);
    RegisterBuiltin("int abs(int)", 0x277b, FUN_10005a70);
    RegisterBuiltin("float fabs(float)", 0x277c, FUN_10005a70);
    RegisterBuiltin("float itof(int)", 0x277d, FUN_10005a70);
    RegisterBuiltin("int ftoi(float)", 0x277e, FUN_10005a70);
    RegisterBuiltin("float asin(float)", 0x277f, FUN_10005a70);
    RegisterBuiltin("float acos(float)", 0x2780, FUN_10005a70);
    RegisterBuiltin("float sinh(float)", 0x2781, FUN_10005a70);
    RegisterBuiltin("float cosh(float)", 0x2782, FUN_10005a70);
    RegisterBuiltin("float tanh(float)", 0x2783, FUN_10005a70);
    RegisterBuiltin("float pow(float,float)", 0x2784, FUN_10005a70);
    RegisterBuiltin("float ceil(float)", 0x2785, FUN_10005a70);
    RegisterBuiltin("float floor(float)", 0x2786, FUN_10005a70);
    RegisterBuiltin("float exp(float)", 0x2787, FUN_10005a70);
    RegisterBuiltin("float log(float)", 0x2788, FUN_10005a70);
    RegisterBuiltin("float log10(float)", 0x2789, FUN_10005a70);
    RegisterBuiltin("float _max(float,float)", 0x278a, FUN_10005a70);
    RegisterBuiltin("float _min(float,float)", 0x278b, FUN_10005a70);
    RegisterBuiltin("int strcmp(string,string)", 0x278c, FUN_10005a70);
    globalBuiltinCount = builtinCount;
    return 0;
}

#include "char_vars.h"
#include "game_vars.h"
#include "script_vars.h"
#include "script_game_builtin.h"

extern "C" void FUN_10017f20(int code, int value);      // the game's parser message callback
// Script_GameBuiltin: script_game_builtin.h (0x10138280)

// 0x1013ad90 (Script_RegisterGameBuiltins). Sets up the game's script parser: the library built-ins, the keyword
// "ch" for the script flags (ch[n]), the message callback and the 93 game built-ins (ids 11000 to 0x2fbe); the names
// declared by then count as global ones.
extern "C" void FUN_1013ad90(void)
{
    DAT_104687b8.InitLibrary();
    DAT_104687b8.m_36734[0] = DAT_10c62edc;
    DAT_104687b8.m_367b4[0] = 0;
    strcpy(DAT_104687b8.m_36334[0], DAT_101dc1fc);
    DAT_104687b8.failed = 0;
    DAT_104687b8.callback = FUN_10017f20;
    DAT_104687b8.RegisterBuiltin("int Message(int,string,string,string,string)", 11000, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int ShopStart()", 0x2b5c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int ShopItem(int)", 0x2b5d, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int ShopEnd()", 0x2b5e, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int ShopEnd2()", 0x2b62, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SelStart(int)", 0x2b5f, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SelText(string,int)", 0x2b60, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SelEnd()", 0x2b61, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Wait(int)", 0x2ee3, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int WaitD(int)", 0x2ee4, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int WaitC(int)", 0x32ca, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int LoadMap(int)", 0x3264, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int CharPos(int,int,int,int)", 0x3265, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int MapEnable(int)", 0x3266, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Lock()", 0x3267, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Unlock()", 0x3268, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Walk(int,int,int)", 0x3269, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Black(int,int,int)", 0x326a, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Look(int,int)", 0x326b, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int BGM(int)", 0x326c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SE(int)", 0x3200, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Event(int)", 0x326d, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Visible(int,int)", 0x326e, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int MapEvent(int)", 0x326f, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int WMessage(string,string)", 0x3270, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Scop(int)", 0x3271, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int PlayMovie(string,int)", 0x3272, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int CrossFade()", 0x3273, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Kaifuku()", 0x3274, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int RCharEffect(int,int)", 0x3275, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Blur(int,int,int)", 0x3276, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int MMV(string)", 0x3277, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Soubi(int)", 0x3278, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int StatusUp(int)", 0x3279, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Camera(int,int,int,int)", 0x327a, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SetRoll(int,int)", 0x327b, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int GetItem(int)", 0x327c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Walk2(int,int,int,int)", 0x3282, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Mes(string)", 0x3283, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Menu(int)", 0x3284, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SetMapColor(int,float,float,float)", 0x3285, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int PostScr(int)", 0x3286, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int LockCommand(int)", 0x3287, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int GetMP()", 0x328a, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SaveMap()", 0x328b, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int RestoreMap()", 0x328c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int LVUP(int)", 0x328d, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SetSavePos()", 0x328e, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int ResetFade()", 0x3291, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int GetGP()", 0x3292, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SetGP(int)", 0x3293, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int KaguTest(int,int)", 0x3296, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int KaguTest2(int,int,string)", 0x3297, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int White(int,int,int)", 0x3298, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int YukaKesi(int,int)", 0x3299, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int REffect(int,int,int,int)", 0x329a, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SendEvent(int)", 0x329b, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int LiftEnable(int)", 0x329c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Lift2C(int)", 0x329e, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Door2C(int)", 0x329f, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int TekiStop(int)", 0x32a0, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int GetCharPos(int,int&,int&,int&)", 0x32a1, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int LiftEnable2(int,int)", 0x32a2, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Getbt15(int)", 0x32a3, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Scop2(int,int,string)", 0x32a4, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SaveCamera()", 0x32a5, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int RestoreCamera()", 0x32a6, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Miss()", 0x32a7, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Reset()", 0x32a8, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SetSavePos2(int,int,int,int)", 0x32a9, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int GAMEOVER(int)", 0x32be, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Recover(int,int,float,float,float)", 0x2ee0, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Special(int)", 0x329d, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Wait2(int)", 0x32c8, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Takara2C(int)", 0x32c9, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Demo(string)", 0x332c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int LoadMapB(int)", 0x332d, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("string StageName(int)", 0x332e, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("string StageMMV(int)", 0x332f, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int ReloadChar()", 0x3330, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int PlayMMV3(string,int)", 0x3336, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int PlayAVI(string,int)", 0x3337, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Fill(int,float)", 0x3338, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Custom()", 0x3339, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("string Custom2(int)", 0x333a, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Custom3(int)", 0x333b, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Ksk(int)", 0x333c, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int SetWaku(int)", 0x333d, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Kabe(string)", 0x333e, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int Mouse(int)", 0x333f, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int PlayMovie2(string,int)", 0x3340, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("string Type(int)", 0x3341, Script_GameBuiltin);
    DAT_104687b8.RegisterBuiltin("int GetCf(int)", 0x2fbe, Script_GameBuiltin);
    DAT_104687b8.globalBuiltinCount = DAT_104687b8.builtinCount;
    DAT_1032d69c = 1;
}

// 0x100558d0 (ListBank::FindCase). Looks for the label `case <value>` of a switch block, starting at `text` (just
// before the block's opening brace) and going on to the end of the script text. Only labels directly inside the
// block count: outside quotes and parentheses and at brace depth 1. The label's number may be followed by spaces,
// tabs or the colon. On a match `found` is set to the character after "case" and 1 is returned; 0 when the block
// ends (or the text does) without one, or on a parse error.
int ListBank::FindCase(const char *value, char *text, char **found)
{
    char word[0x100];
    int quoted = 0;
    int parens = 0;
    int braces = 0;
    int opened = 0;
    int count = 0;
    for (; text < &this->text[textLength]; text++)
    {
        if (failed != 0)
            return 0;
        if (quoted == 0 && parens == 0 && braces == 1 && count >= 3 && text[-3] == 'c' && text[-2] == 'a' &&
            text[-1] == 's' && text[0] == 'e')
        {
            // `text` moves past "case" (to the character after it)
            text++;
            char *label = text;
            ReadWord(&label, word);
            for (int i = (int)strlen(word) - 1; i > 0; i--)
            {
                if (word[i] == ' ' || word[i] == '	' || word[i] == ':')
                    word[i] = 0;
            }
            if (atol(value) == atol(word))
            {
                *found = text;
                return 1;
            }
            text += 2;
        }
        if (*text == '(')
            parens++;
        else if (*text == ')')
            parens--;
        if (*text == '"')
        {
            if (quoted == 1)
                quoted = 0;
            else if (quoted == 0)
                quoted = 1;
        }
        if (quoted == 0 && parens == 0)
        {
            if (*text == '{')
            {
                braces++;
                opened = 1;
            }
            if (*text == '}')
                braces--;
        }
        if (opened != 0 && braces == 0)
            return 0;
        count++;
    }
    return 0;
}

// 0x10055aa0 (ListBank::FindLabel). Looks for the label `name:` (for goto) directly inside the block that starts at
// `text`, the same way FindCase looks for `case <value>`: outside quotes and parentheses and at brace depth 1, with
// the colon right after the name. The word read there may end in spaces, tabs or the colon. On a match `found` is
// set to the label's first character and 1 is returned; 0 when the block or the text ends, or on a parse error.
int ListBank::FindLabel(const char *name, char *text, char **found)
{
    char word[0x100];
    int quoted = 0;
    int parens = 0;
    int braces = 0;
    int opened = 0;
    int count = 0;
    int length = strlen(name);
    for (; text < &this->text[textLength]; text++)
    {
        if (failed != 0)
            return 0;
        if (quoted == 0 && parens == 0 && braces == 1 && count >= length - 1 && text[length + 1] == ':')
        {
            text++;
            char *label = text;
            ReadWord(&label, word);
            for (int i = (int)strlen(word) - 1; i > 0; i--)
            {
                if (word[i] == ' ' || word[i] == '	' || word[i] == ':')
                    word[i] = 0;
            }
            if (strcmp(name, word) == 0)
            {
                *found = text;
                return 1;
            }
            text += 2;
        }
        if (*text == '(')
            parens++;
        else if (*text == ')')
            parens--;
        if (*text == '"')
        {
            if (quoted == 1)
                quoted = 0;
            else if (quoted == 0)
                quoted = 1;
        }
        if (quoted == 0 && parens == 0)
        {
            if (*text == '{')
            {
                braces++;
                opened = 1;
            }
            if (*text == '}')
                braces--;
        }
        if (opened != 0 && braces == 0)
            return 0;
        count++;
    }
    return 0;
}

// "終わりまで来た。" ("Reached the end."), Shift-JIS.
#define END_REACHED_TEXT "IíèÜÅ½B"

// 0x10055ca0 (ListBank::SkipStatement). Moves `*text` past one statement: an `if` (with its `else` branch, both
// skipped recursively), a `for`, `while` or `switch` statement, a `{ }` block or a plain statement ending in ';'
// (outside quotes, parentheses and braces). Returns 1; reports an error and returns 0 when the script text ends
// first (0 without a message after an earlier error).
int ListBank::SkipStatement(char **text)
{
    char word[0x100];
    char *p = *text;
    int quoted = 0;
    int parens = 0;
    int count = 0;
    int braces = 0;
    int opened = 0;
    char *end = &this->text[textLength];
    for (; p < end; p++, count++)
    {
        if (failed != 0)
            return 0;
        if (quoted == 0 && parens == 0 && braces == 0)
        {
            if (count >= 1 && p[-1] == 'i' && p[0] == 'f')
            {
                // skip the condition and the statement, then an `else` branch if one follows
                *text = p;
                SkipStatement(text);
                char *after = *text;
                char *next = after;
                ReadWord(&next, word);
                if (strcmp(word, "else") == 0)
                {
                    *text = next;
                    SkipStatement(text);
                    after = *text;
                }
                *text = after;
                return 1;
            }
            if ((count >= 2 && p[-2] == 'f' && p[-1] == 'o' && p[0] == 'r') ||
                (count >= 4 && p[-4] == 'w' && p[-3] == 'h' && p[-2] == 'i' && p[-1] == 'l' && p[0] == 'e') ||
                (count >= 5 && p[-5] == 's' && p[-4] == 'w' && p[-3] == 'i' && p[-2] == 't' && p[-1] == 'c' &&
                 p[0] == 'h'))
            {
                *text = p;
                SkipStatement(text);
                return 1;
            }
        }
        if (*p == '(')
            parens++;
        else if (*p == ')')
            parens--;
        if (*p == '"')
        {
            if (quoted == 1)
                quoted = 0;
            else if (quoted == 0)
                quoted = 1;
        }
        if (quoted == 0 && parens == 0)
        {
            if (*p == '{')
            {
                braces++;
                opened = 1;
            }
            if (*p == '}')
                braces--;
            if (*p == ';' && braces == 0)
            {
                *text = p + 1;
                return 1;
            }
        }
        if (opened != 0 && braces == 0)
        {
            *text = p + 1;
            return 1;
        }
    }
    ReportError(END_REACHED_TEXT);
    *text = p;
    return 0;
}

#include "script_value.h"

// "定義されてない識別子です。" ("This identifier is not defined."), Shift-JIS.
#define UNDEFINED_NAME_TEXT "è`³êÄÈ¢¯ÊqÅ·B"

// 0x10037560 (ListBank::AssignValue). Stores `value` (of `type`) into the variable `name`: an element of a
// registered keyword array (int or float elements, the index from the name's first [ ]), or a declared script
// variable (int, float or text; text receives numbers formatted as "%d" / "%f" in quotes, and numbers parse the
// quoted text after its quote). Values are converted to the variable's type. Reports an undefined name. Returns 1
// once the variable was found, 0 otherwise or after an error.
int ListBank::AssignValue(const char *name, const ScriptValue *value, char type)
{
    if (failed != 0)
        return 0;
    ScriptValue v = *value;
    {
        int index;
        int element;
        if (M_100048f0(name, &index) != 0)
        {
            FirstIndexValue(name, &element);
            if (m_367b4[index] == 0)
            {
                if (type == 0)
                {
                    m_36734[index][element] = v.i();
                    return 1;
                }
                if (type == 1)
                {
                    m_36734[index][element] = (int)v.f();
                    return 1;
                }
            }
            else
            {
                if (type == 0)
                {
                    ((float *)m_36734[index])[element] = (float)v.i();
                    return 1;
                }
                if (type == 1)
                    ((float *)m_36734[index])[element] = (float)(int)v.f();
            }
            return 1;
        }
    }
    {
        int index;
        int group;
        int element;
        if (FindVariable(name, &index, &group, &element) != 0)
        {
            if (variableTypes[group][index] == 0)
            {
                if (type == 0)
                {
                    values[group][index].ints[element] = v.i();
                    return 1;
                }
                if (type == 1)
                {
                    values[group][index].ints[element] = (int)v.f();
                    return 1;
                }
                if (type == 2)
                {
                    values[group][index].ints[element] = atol(&v.text[1]);
                    return 1;
                }
            }
            else if (variableTypes[group][index] == 1)
            {
                if (type == 0)
                {
                    values[group][index].floats[element] = (float)v.i();
                    return 1;
                }
                if (type == 1)
                {
                    values[group][index].floats[element] = v.f();
                    return 1;
                }
                if (type == 2)
                {
                    values[group][index].floats[element] = (float)atof(&v.text[1]);
                    return 1;
                }
            }
            else if (variableTypes[group][index] == 2)
            {
                if (type == 0)
                {
                    sprintf(values[group][index].texts[element], "\"%d\"", v.i());
                    return 1;
                }
                if (type == 1)
                {
                    sprintf(values[group][index].texts[element], "\"%f\"", v.f());
                    return 1;
                }
                if (type == 2)
                    strcpy(values[group][index].texts[element], v.text);
            }
            return 1;
        }
    }
    ReportError(UNDEFINED_NAME_TEXT);
    return 0;
}

#define DUPLICATE_NAME_TEXT "\x95\xcf\x90\x94\x96\xbc\x82\xaa\x8f" "d\x95\xa1\x82\xb5\x82\xc4\x82\xdc\x82\xb7\x81" "B"

// 0x100ed510 (ListBank::DeclareFromText). Declares the variables listed in `declaration` in group `group`: each
// word is a variable name with optional "[size]" parts (every size is compiled; their product is the element
// count). The type comes from ReadType. A name that is already declared in the group is an error. Each variable gets
// storage for all its elements (zeroed numbers, or text elements preset to ""), and its sizes, dimension count and
// type are recorded. Returns 1 when the list is used up, 0 after an error.
int ListBank::DeclareFromText(char *declaration, int group)
{
    char *source = declaration;
    char type = ReadType(&source);
    int elements = 1;
    char word[LISTBANK_TEXT_SIZE];
    while (failed == 0)
    {
        char part[LISTBANK_TEXT_SIZE];
        ReadWord(&source, word);
        if (word[0] == 0)
            return 1;
        borrowed[group][variableCounts[group]] = 0;
        if (GetIndexPart(word, part, 0) != 0)
        {
            int dimension = 0;
            while (GetIndexPart(word, part, dimension) != 0)
            {
                CompileCall(part, 0);
                if (resultType == 0)
                {
                    // already an int
                }
                else if (resultType == 1)
                    *result = (int)*(float *)result;
                else if (resultType == 2)
                    *result = atol((char *)result + 1);
                else
                    *result = 0;
                sizes[group][variableCounts[group]][dimension] = *result;
                elements = sizes[group][variableCounts[group]][dimension] * elements;
                dimension++;
                sizes[group][variableCounts[group]][dimension] = 0;
            }
            dimensions[group][variableCounts[group]] = dimension;
            int length = 0;
            while (word[length] != '[')
                length++;
            word[length] = 0;
            strcpy(variableNames[group][variableCounts[group]], word);
        }
        else
        {
            strcpy(variableNames[group][variableCounts[group]], word);
            sizes[group][variableCounts[group]][0] = 0;
            dimensions[group][variableCounts[group]] = 0;
        }
        for (int i = 0; i < variableCounts[group]; i++)
        {
            if (strcmp(variableNames[group][i], word) == 0)
            {
                failed = 1;
                printf("%s\n", DUPLICATE_NAME_TEXT);
                if (callback != 0)
                    callback(1, (int)DUPLICATE_NAME_TEXT);
                if (callback != 0)
                    callback(5000, cursor[depth] - text);
                FreeAll();
                return 0;
            }
        }
        if (type == 0)
        {
            values[group][variableCounts[group]].ints = new int[elements];
            values[group][variableCounts[group]].ints[0] = 0;
            for (int i = 0; i < elements; i++)
                values[group][variableCounts[group]].ints[i] = 0;
        }
        else if (type == 1)
        {
            values[group][variableCounts[group]].floats = new float[elements];
            values[group][variableCounts[group]].ints[0] = 0;
            for (int i = 0; i < elements; i++)
                values[group][variableCounts[group]].ints[i] = 0;
        }
        else if (type == 2)
        {
            values[group][variableCounts[group]].texts = new char[elements][LISTBANK_TEXT_SIZE];
            for (int i = 0; i < elements; i++)
                strcpy(values[group][variableCounts[group]].texts[i], "\"\"");
        }
        variableTypes[group][variableCounts[group]] = type;
        variableCounts[group]++;
    }
    return 0;
}

#define NO_INTERPRETER_TEXT "\x83" "C\x83\x93\x83^\x83v\x83\x8a\x83^\x82\xc5\x82\xcd\x8eg\x97p\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define NO_GOTO_TEXT "goto\x82\xcd\x8eg\x97p\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define NO_FUNCTION_DEFINITION_TEXT "\x83" "C\x83\x93\x83^\x83v\x83\x8a\x83^\x82\xc5\x82\xcd\x8a\xd6\x90\x94\x92\xe8\x8b`\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define NO_LOCAL_FUNCTION_TEXT "\x83\x8d\x81[\x83J\x83\x8b\x8a\xd6\x90\x94\x82\xcd\x92\xe8\x8b`\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define BAD_FOR_TEXT "for\x82\xcc;\x82\xaa\x82\xa8\x82\xa9\x82\xb5\x82\xa2\x81" "B"

// 0x100ed9b0 (ListBank::CompileStatement). Classifies one statement by its first word. A label (ends in ':') and
// `case` give 1; return, break, continue, if, for, while and switch give their STATEMENT_ code, with the
// condition (or the three `for` parts) read into statementParts[depth]; all of these are errors outside
// CompileText ("the interpreter cannot use it"). goto is always an error. int/float/string declare variables in the
// current level (a function definition is an error there; at level 0 it goes to DeclareFunction). Any other
// statement is compiled as a call. Returns the statement kind, 0 after an error.
int ListBank::CompileStatement(char *statement)
{
    char *cursorText = statement;
    char word[LISTBANK_TEXT_SIZE];
    ReadWord(&cursorText, word);
    int length = strlen(word);
    if (length == 0)
        return 0;
    if (word[length - 1] == ':')
        return 1;
    if (strcmp(word, "return") == 0)
    {
        if (compiling != 0)
        {
            while (*cursorText == ' ' || *cursorText == '\t' || *cursorText == '\n' || *cursorText == '\r')
                cursorText++;
            if (*cursorText == '(')
                cursorText++;
            ReadWord(&cursorText, statementParts[depth][0]);
            return STATEMENT_RETURN;
        }
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "break") == 0)
    {
        if (compiling != 0)
            return STATEMENT_BREAK;
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "continue") == 0)
    {
        if (compiling != 0)
            return STATEMENT_CONTINUE;
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "case") == 0)
    {
        if (compiling != 0)
            return 1;
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "if") == 0)
    {
        if (compiling != 0)
        {
            while (*cursorText != '(')
                cursorText++;
            cursorText++;
            ReadStatement(&cursorText, statementParts[depth][0]);
            return STATEMENT_IF;
        }
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "for") == 0)
    {
        if (compiling == 0)
        {
            ReportError(NO_INTERPRETER_TEXT);
            return 0;
        }
        while (*cursorText != '(')
            cursorText++;
        cursorText++;
        ReadStatement(&cursorText, statementParts[depth][0]);
        if (statementParts[depth][0][0] != ';')
        {
            ReadStatement(&cursorText, statementParts[depth][1]);
            if (statementParts[depth][1][0] != ';')
                goto badFor;
        }
        ReadStatement(&cursorText, statementParts[depth][1]);
        if (statementParts[depth][1][0] != ';')
        {
            ReadStatement(&cursorText, statementParts[depth][2]);
            if (statementParts[depth][2][0] != ';')
            {
            badFor:
                ReportError(BAD_FOR_TEXT);
                return 0;
            }
        }
        else
            strcpy(statementParts[depth][1], "1");
        ReadStatement(&cursorText, statementParts[depth][2]);
        return STATEMENT_FOR;
    }
    if (strcmp(word, "while") == 0)
    {
        if (compiling != 0)
        {
            while (*cursorText != '(')
                cursorText++;
            cursorText++;
            ReadStatement(&cursorText, statementParts[depth][0]);
            return STATEMENT_WHILE;
        }
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "switch") == 0)
    {
        if (compiling != 0)
        {
            while (*cursorText != '(')
                cursorText++;
            cursorText++;
            ReadStatement(&cursorText, statementParts[depth][0]);
            return STATEMENT_SWITCH;
        }
        ReportError(NO_INTERPRETER_TEXT);
        return 0;
    }
    if (strcmp(word, "goto") == 0)
    {
        ReportError(NO_GOTO_TEXT);
        return 0;
    }
    if (strcmp(word, "int") == 0)
    {
        if (M_100049e0(statement) == 0)
        {
            DeclareFromText(statement, depth);
            return 1;
        }
    }
    else
    {
        if (strcmp(word, "float") != 0 && strcmp(word, "string") != 0)
        {
            strcpy(word, statement);
            CompileCall(word, 0);
            return 1;
        }
        if (M_100049e0(statement) == 0)
        {
            DeclareFromText(statement, depth);
            return 1;
        }
    }
    if (compiling == 0)
        ReportError(NO_FUNCTION_DEFINITION_TEXT);
    else if (depth == 0)
        DeclareFunction(statement);
    else
        ReportError(NO_LOCAL_FUNCTION_TEXT);
    return 1;
}

// "関数が多すぎ" ("too many functions"), Shift-JIS.
#define TOO_MANY_FUNCTIONS_TEXT "\x8a\xd6\x90\x94\x82\xaa\x91\xbd\x82\xb7\x82\xac"
// "対応する括弧がありません。" ("There is no matching bracket."), Shift-JIS.
#define NO_MATCHING_BRACKET_TEXT "\x91\xce\x89\x9e\x82\xb7\x82\xe9\x8a\x87\x8c\xca\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb9\x82\xf1\x81" "B"
// "引数に異常な記述があります。" ("The argument has a wrong description."), Shift-JIS.
#define BAD_ARGUMENT_TEXT "\x88\xf8\x90\x94\x82\xc9\x88\xd9\x8f\xed\x82\xc8\x8b" "L\x8f" "q\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb7\x81" "B"
// "型が異常です。" ("The type is wrong."), Shift-JIS.
#define BAD_PARAMETER_TYPE_TEXT "\x8c^\x82\xaa\x88\xd9\x8f\xed\x82\xc5\x82\xb7\x81" "B"
// "関数名が重複してます。" ("The function name is a duplicate."), Shift-JIS.
#define DUPLICATE_FUNCTION_TEXT "\x8a\xd6\x90\x94\x96\xbc\x82\xaa\x8f" "d\x95\xa1\x82\xb5\x82\xc4\x82\xdc\x82\xb7\x81" "B"
// Longest function table (the parser keeps 0x100 functions) and the most parameters of one function.
#define FUNCTION_LIMIT 0xff
#define PARAMETER_LIMIT 0x10
// builtinIds of a function declared in the script text (builtins have their positive ids).
#define SCRIPT_FUNCTION_ID -1
// Placeholder name of a parameter that has none (the bytes "tmp").
#define UNNAMED_PARAMETER "tmp"
// builtinParamTypes: the parameter is passed by reference (added to the type).
#define BY_REFERENCE 10

// Reports an error exactly like ReportError (which the original compiler expanded here three times).
static __forceinline int ReportErrorExpanded(ListBank *bank, const char *message)
{
    bank->failed = 1;
    printf("%s\n", message);
    if (bank->callback != 0)
        bank->callback(1, (int)message);
    if (bank->callback != 0)
        bank->callback(5000, bank->cursor[bank->depth] - bank->text);
    bank->FreeAll();
    return 0;
}

// 0x100366c0 (ListBank::DeclareFunction). Declares a function written in the script, `statement` being its header
// "type name(type name, &type name, ...)": the function table gets the body text (the read position of level 0),
// the return type, the name and the parameter types and names (a "&" before a parameter name marks it as passed by
// reference). A parameter without a name gets "tmp", a name equal to a type word reports a wrong argument, and a
// name already declared with the same parameter types is a duplicate. Returns 1 when declared, 0 after an error
// (the missing-bracket errors also clear the last result).
int ListBank::DeclareFunction(char *statement)
{
    // The scope of the header parsing: its buffers share their stack space with the loop below.
    {
        char *source = statement;
        char params[LISTBANK_TEXT_SIZE];
        char word[LISTBANK_TEXT_SIZE];
        if (builtinCount > FUNCTION_LIMIT)
            MessageBoxA(0, TOO_MANY_FUNCTIONS_TEXT, "err", 0);
        builtinHandlers[builtinCount].body = cursor[0];
        builtinIds[builtinCount] = SCRIPT_FUNCTION_ID;
        builtinReturnTypes[builtinCount] = ReadType(&source);
        ReadWord(&source, builtinNames[builtinCount]);
        for (int j = 0; j < PARAMETER_LIMIT; j++)
            builtinParamTypes[builtinCount][j] = (char)0xff;
        strcpy(params, source);
        source = params;
        if (params[0] != '(')
        {
            ReportErrorExpanded(this, NO_MATCHING_BRACKET_TEXT);
            self->resultType = 0;
            *self->result = 0;
            return 0;
        }
        params[0] = ' ';
        if (params[strlen(params) - 1] != ')')
        {
            ReportError(NO_MATCHING_BRACKET_TEXT);
            self->resultType = 0;
            *self->result = 0;
            return 0;
        }
        params[strlen(params) - 1] = ' ';
        int i = 0;
        builtinParamTypes[builtinCount][0] = (char)0xff;
        while (i < PARAMETER_LIMIT)
        {
            if (failed != 0)
                return 0;
            ReadTerm(&source, word);
            builtinParamCounts[builtinCount] = (char)i;
            if (word[0] == 0)
                break;
            if (strcmp(word, "int") == 0)
                builtinParamTypes[builtinCount][i] = 0;
            else if (strcmp(word, "float") == 0)
                builtinParamTypes[builtinCount][i] = 1;
            else if (strcmp(word, "string") == 0)
                builtinParamTypes[builtinCount][i] = 2;
            else
            {
                builtinParamTypes[builtinCount][i] = 0;
                ReportError(BAD_PARAMETER_TYPE_TEXT);
            }
            for (;;)
            {
                ReadTerm(&source, builtinParamNames[builtinCount][i]);
                if (builtinParamNames[builtinCount][i][0] == '&')
                {
                    builtinParamTypes[builtinCount][i] += BY_REFERENCE;
                    ReadTerm(&source, builtinParamNames[builtinCount][i]);
                }
                if (builtinParamNames[builtinCount][i][0] == 0)
                {
                    ReportErrorExpanded(this, BAD_ARGUMENT_TEXT);
                    strcpy(builtinParamNames[builtinCount][i], UNNAMED_PARAMETER);
                    break;
                }
                if (strcmp(builtinParamNames[builtinCount][i], "int") != 0 &&
                    strcmp(builtinParamNames[builtinCount][i], "float") != 0 &&
                    strcmp(builtinParamNames[builtinCount][i], "string") != 0)
                    break;
                ReportErrorExpanded(this, BAD_ARGUMENT_TEXT);
                i++;
                if (strcmp(builtinParamNames[builtinCount][i], "int") == 0)
                    builtinParamTypes[builtinCount][i] = 0;
                else if (strcmp(builtinParamNames[builtinCount][i], "float") == 0)
                    builtinParamTypes[builtinCount][i] = 1;
                else if (strcmp(builtinParamNames[builtinCount][i], "string") == 0)
                    builtinParamTypes[builtinCount][i] = 2;
                strcpy(builtinParamNames[builtinCount][i - 1], UNNAMED_PARAMETER);
            }
            i++;
            builtinParamCounts[builtinCount] = (char)i;
            builtinParamTypes[builtinCount][i] = (char)0xff;
        }
    }
    for (int k = 0; k < builtinCount; k++)
    {
        if (strcmp(builtinNames[k], builtinNames[builtinCount]) == 0 &&
            memcmp(builtinParamTypes[builtinCount], builtinParamTypes[k], PARAMETER_LIMIT) == 0)
        {
            ReportError(DUPLICATE_FUNCTION_TEXT);
            return 0;
        }
    }
    builtinCount++;
    return 1;
}
