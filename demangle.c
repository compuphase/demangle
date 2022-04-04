/* GNU C++ symbol name demangler
 *
 * Copyright 2022, CompuPhase
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <assert.h>
#include <alloca.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demangle.h"

#define sizearray(a)        (sizeof(a) / sizeof((a)[0]))
#define MAX_SUBSTITUTIONS   20
#define MAX_TEMPLATE_SUBST  10
#define MAX_FUNC_NESTING    5

struct mangle {
  int valid;            /**< whether the mangled name is valid */
  char *plain;          /**< [output] demangled name */
  size_t size;          /**< size (in characters) of the "plain" buffer */
  const char *mangled;  /**< [input] mangled name */
  const char *mpos;     /**< current position, look-ahead pointer */
  short nest;           /**< nesting level for names */
  unsigned char is_typecast_op; /**< flag: is this a typecast operator */
  char qualifiers[8];   /**< const, reference, and others */
  short func_nest;      /**< function nesting level (of parameter lists) */
  char *parameter_base[MAX_FUNC_NESTING];
  char *substitions[MAX_SUBSTITUTIONS];
  size_t subst_count;
  char *tpl_subst[MAX_TEMPLATE_SUBST];
  size_t tpl_subst_count;
};

static int is_operator(struct mangle *mangle);
static int is_builtin_type(struct mangle *mangle);
static int is_abbreviation(struct mangle *mangle);

static void _template_args(struct mangle *mangle);
static void _source_name(struct mangle *mangle);
static void _unqualified_name(struct mangle *mangle);
static void _decltype(struct mangle *mangle);
static void _function_type(struct mangle *mangle);
static void _substitution(struct mangle *mangle);
static void _template_param(struct mangle *mangle);
static void _unnamed_type_name(struct mangle *mangle);
static void _local_name(struct mangle *mangle);
static void _ctor_dtor_name(struct mangle *mangle);
static void _operator(struct mangle *mangle);
static void _expr_primary(struct mangle *mangle);
static void _nested_name(struct mangle *mangle);
static void _name(struct mangle *mangle);
static void _type(struct mangle *mangle);
static void _function_encoding(struct mangle *mangle);
static void _encoding(struct mangle *mangle);

struct stringpair {
  const char *abbrev;
  const char *name;
};

static const struct stringpair operators[] = {
  { "cv", "(?)" },              /* type cast */
  { "nw", "new" },
  { "na", "new[]" },
  { "dl", "delete" },
  { "da", "delete[]" },
  { "ng", "-" },                /* (unary) */
  { "ad", "&" },                /* (unary) */
  { "de", "*" },                /* (unary) */
  { "co", "~" },
  { "pl", "+" },
  { "mi", "-" },
  { "ml", "*" },
  { "dv", "/" },
  { "rm", "%" },
  { "an", "&" },
  { "or", "|" },
  { "eo", "^" },
  { "aS", "=" },
  { "pL", "+=" },
  { "mI", "-=" },
  { "mL", "*=" },
  { "dV", "/=" },
  { "rM", "%=" },
  { "aN", "&=" },
  { "oR", "|=" },
  { "eO", "^=" },
  { "ls", "<<" },
  { "rs", ">>" },
  { "lS", "<<=" },
  { "rS", ">>=" },
  { "eq", "==" },
  { "ne", "!=" },
  { "lt", "<" },
  { "gt", ">" },
  { "le", "<=" },
  { "ge", ">=" },
  { "ss", "<=>" },
  { "nt", "!" },
  { "aa", "&&" },
  { "oo", "||" },
  { "pp", "++" },               /* (postfix in <expression> context) */
  { "mm", "--" },               /* (postfix in <expression> context) */
  { "cm", "," },
  { "pm", "->*" },
  { "pt", "->" },
  { "cl", "()" },
  { "ix", "[]" },
  { "qu", "?" },
};

static const struct stringpair types[] = {
  { "v", "void" },
  { "w", "wchar_t" },
  { "b", "bool" },
  { "c", "char" },
  { "a", "signed char" },
  { "h", "unsigned char" },
  { "s", "short" },
  { "t", "unsigned short" },
  { "i", "int" },
  { "j", "unsigned int" },
  { "l", "long" },
  { "m", "unsigned long" },
  { "x", "long long" },         /* __int64 */
  { "y", "unsigned long long" },/* __int64 */
  { "n", "__int128" },
  { "o", "unsigned __int128" },
  { "f", "float" },
  { "d", "double" },
  { "e", "long double" },       /* __float80 */
  { "g", "__float128" },
  { "z", "ellipsis" },
  { "Da","auto" },
  { "Dc","decltype(auto)" },
  { "Dn","std::nullptr_t" },    /* i.e., decltype(nullptr) */
  { "Dh","decimal16" },
  { "Df","decimal32" },
  { "Dd","decimal64" },
  { "De","decimal128" },
  { "Du","char8_t" },
  { "Ds","char16_t" },
  { "Di","char32_t" },
};

static const struct stringpair abbreviations[] = {
  { "St", "std" },              /* also ::std:: */
  { "Sa", "std::allocator" },
  { "Sb", "std::basic_string" },
  { "Ss", "std::string" },      /* std::basic_string<char,::std::char_traits<char>,::std::allocator<char>>*/
  { "Si", "std::istream" },     /* std::basic_istream<char,std::char_traits<char>> */
  { "So", "std::ostream" },     /* std::basic_ostream<char,std::char_traits<char>> */
  { "Sd", "std::iostream" },    /* std::basic_iostream<char,std::char_traits<char>> */
};

/** peek() - match, but don't change the current position. */
static int peek(struct mangle *mangle, const char *keyword)
{
  assert(mangle != NULL);
  return mangle->valid && strncmp(mangle->mpos, keyword, strlen(keyword)) == 0;
}

/** match() - advance the current position on a match (do not move on
 *  mismatch). Never matches anything after the mangled name has been flagged as
 *  invalid.
 */
static int match(struct mangle *mangle, const char *keyword)
{
  assert(mangle != NULL);
  int result = peek(mangle, keyword);
  if (result)
    mangle->mpos += strlen(keyword);
  return result;
}

/** expect() - advance (skip) on match, but flag as invalid on mismatch. */
static int expect(struct mangle *mangle, const char *keyword)
{
  assert(mangle != NULL);
  if (mangle->valid && !match(mangle, keyword))
    mangle->valid = 0;
  return mangle->valid;
}

/** on_sentinel() - returns true if arrived at the end of the mangled symbol. */
static int on_sentinel(struct mangle *mangle)
{
  assert(mangle != NULL);
  return !mangle->valid
         || *mangle->mpos == '\0'
         || *mangle->mpos == '.'                                  /* clone suffix */
         || (*mangle->mpos == '@' && *(mangle->mpos + 1) == '@'); /* library suffix */
}

static const char *find_matching(const char *head, const char *tail, char c)
{
  assert(head != NULL);
  assert(tail != NULL && tail >= head);
  int dir;
  char m;
  switch (c) {
  case '(':
    m = ')';
    dir = 1;
    break;
  case ')':
    m = '(';
    dir = -1;
    break;
  case '[':
    m = ']';
    dir = 1;
    break;
  case ']':
    m = '[';
    dir = -1;
    break;
  case '<':
    m = '>';
    dir = 1;
    break;
  case '>':
    m = '<';
    dir = -1;
    break;
  default:
    assert(0);
  }
  int nest = 0;
  const char *iter;
  if (dir < 0) {
    iter = tail;
    while (iter != head && (*iter != m || nest > 0)) {
      iter -= 1;
      if (*iter == c)
        nest++;
      else if (*iter == m)
        nest--;
    }
  } else {
    iter = head;
    while (iter != tail && (*iter != m || nest > 0)) {
      iter += 1;
      if (*iter == c)
        nest++;
      else if (*iter == m)
        nest--;
    }
  }
  return (*iter == m) ? iter : NULL;
}

static char *check_func_array(struct mangle *mangle, const char *base)
{
  assert(mangle != NULL);
  assert(base != NULL && base >= mangle->plain);
  if (!mangle->valid || strlen(base) == 0)
    return NULL;
  /* go to the end (either of the string, or of the parenthesized section) */
  const char *p = base + strlen(base) - 1;
  if (*base == '(') {
    p = find_matching(base, p, *base);
    assert(p != NULL);  /* otherwise, constructed plain string was invalid */
    p -= 1;             /* point to last character before matching ')' */
  }
  if (p >= mangle->plain + 5 && strcmp(p - 4, "const") == 0)
    p -= 5;
  if (p > mangle->plain && *p == ' ')
    p -= 1;
  if (*p == ')') {
    p = find_matching(mangle->plain, p, *p);
    assert(p != NULL && *p == '(');
  } else if (*p == ']') {
    while (*p == ']') {
      p = find_matching(mangle->plain, p,*p);
      assert(p != NULL && *p == '[');
      if (p > base && *(p - 1) == ']')
        p -= 1;
    }
  }
  return (p >= base && (*p == '(' || *p == '[')) ? (char*)p : NULL;
}

static char *insertion_point(struct mangle *mangle, const char *base)
{
  /* find the most deeply nested "(*" or "(..::*)", skipping templates */
  const char *mark = base;
  const char *post_mark = mark;
  int advance = 0;
  for ( ;; ) {
    const char *head = mark + advance;
    while (*head != '\0') {
      if (*head == '(')
        break;
      if (*head == '<') {
        while (*head != '\0' && *head != '>')
          head++;
      }
      if (*head != '\0')
        head++;
    }
    if (*head != '(')
      break;
    const char *tail = head + 1;
    if (*tail == '*') {
      while (*(tail + 1) == '*')
        tail++;
    } else if (isalpha(*tail) || *tail == '_') {
      while (*tail != '\0' && *tail != ')' && *tail != ':')
        tail++;
      if (*tail == ':' && *(tail + 1) == ':' && *(tail + 2) == '*') {
        tail += 2;
        while (*(tail + 1) == '*')
          tail++;
      }
    }
    if (*head != '(' || *tail != '*')
      break;
    mark = head;
    post_mark = tail;
    advance = 1;
  }

  /* if a function definition is enclosed in it, get the insertion point from it;
     otherwise skip any '*' characters */
  char *p = check_func_array(mangle, mark);
  if (p == NULL) {
    if (*mark == '(' && *post_mark == '*')
      p = (char*)post_mark + 1;
    else
      p = (char*)((mark == base) ? base + strlen(base) : mark);
  }

  return p;
}

/** get_number() - extracts the number, but does not interpret it (the number
 *  is simply stored as a string).
 */
static size_t get_number(struct mangle *mangle, char *field, size_t size, int hex)
{
  assert(mangle != NULL);
  assert(field != NULL);
  assert(size > 0);
  memset(field, 0, size);
  size_t i = 0;
  while (isdigit(*mangle->mpos) || (hex && isxdigit(*mangle->mpos))) {
    if (i < size - 1)
      field[i] = *mangle->mpos++;
    i++;
  }
  return i;
}

/** append() - appends text at the end of the result string (demangled string).
 *  If the text would not fit, the result is set to invalid.
 */
static void append(struct mangle *mangle, const char *text)
{
  assert(mangle != NULL);
  assert(text != NULL);
  if (mangle->valid) {
    size_t len = strlen(mangle->plain);
    if (len + strlen(text) < mangle->size)
      strcpy(mangle->plain + len, text);
    else
      mangle->valid = 0;
  }
}

/** append_space() adds a space to the result string, unless the character
 *  currently at the end is a separator too. (This still adds more spaces than
 *  strictly necessary, but it avoids glueing words together.)
 */
static void append_space(struct mangle *mangle)
{
  /* optionally appends a space character */
  assert(mangle != NULL);
  size_t len = strlen(mangle->plain);
  if (len > 0) {
    const char separators[]= " ([<,:";
    if (strchr(separators, mangle->plain[len - 1]) == NULL)
      append(mangle, " ");
  }
}

static void insert(struct mangle *mangle, char *mark, const char *text)
{
  assert(mangle != NULL);
  assert(text != NULL);

  if (mangle->valid) {
    assert(mark >= mangle->plain && mark <= mangle->plain + strlen(mangle->plain));
    if (*mark == '\0') {
      /* inserting at the end is appending */
      append(mangle, text);
    } else {
      size_t len = strlen(mangle->plain);
      size_t ln2 = strlen(text);
      assert(ln2 > 0);
      if (len + ln2 < mangle->size) {
        size_t num = len - (mark - mangle->plain) + 1;
        memmove((char*)mark + ln2, mark, num * sizeof(char));
        memmove((char*)mark, text, ln2 * sizeof(char));
      }
    }
  }
}

static char *current_position(struct mangle *mangle)
{
  assert(mangle != NULL);
  return mangle->plain + strlen(mangle->plain);
}

static void add_substitution(struct mangle *mangle, const char *text, int tpl)
{
  assert(mangle != NULL);
  assert(text != NULL);

  if (!mangle->valid)
    return;

  /* duplicate substitutions are not merged (the Itanium ABI documentation
     implies that they are) */
  #if 0
    if (!tpl) {
      for (int i = 0; i < mangle->subst_count; i++) {
        if (strcmp(text, mangle->substitions[i]) == 0)
          return; /* substition already exists, do not add again */
      }
    }
  #endif

  size_t length = strlen(text);
  char *str = malloc((length + 1) * sizeof(char));
  if (str != NULL) {
    memcpy(str, text, length);
    str[length] = '\0';
    if (tpl) {
      assert(mangle->tpl_subst_count < MAX_TEMPLATE_SUBST);
      if (mangle->subst_count < MAX_TEMPLATE_SUBST) {
        mangle->tpl_subst[mangle->tpl_subst_count] = str;
        mangle->tpl_subst_count += 1;
      }
    } else {
      assert(mangle->subst_count < MAX_SUBSTITUTIONS);
      if (mangle->subst_count < MAX_SUBSTITUTIONS) {
        mangle->substitions[mangle->subst_count] = str;
        mangle->subst_count += 1;
      }
    }
  }
}

static void free_tpl_subst(struct mangle *mangle, size_t top)
{
  assert(mangle != NULL);
  assert(top <= mangle->tpl_subst_count);
  for (size_t i = 0; i < top; i++) {
    assert(mangle->tpl_subst[i] != NULL);
    free((void*)mangle->tpl_subst[i]);
    mangle->tpl_subst[i] = NULL;
  }
}

/** _qualifier_pre() handles <cv-qualifier> plus optionally <ref-qualifier>, but
 *  stores codes in a list (because these need to be appended after the type).
 */
static void _qualifier_pre(struct mangle *mangle, char *qualifiers, size_t size, int include_ref)
{
  assert(mangle != NULL);
  assert(qualifiers != NULL);
  assert(size > 0);
  size_t count = 0;
  while (count < size - 1 && (*mangle->mpos == 'r' || *mangle->mpos == 'V' || *mangle->mpos == 'K')) {
    qualifiers[count++] = *mangle->mpos;
    mangle->mpos += 1;
  }
  if (include_ref) {
    while (count < size - 1 && (*mangle->mpos == 'R' || *mangle->mpos == 'O')) {
      qualifiers[count++] = *mangle->mpos;
      mangle->mpos += 1;
    }
  }
  assert(count < size);
  qualifiers[count] = '\0';
}

static void _qualifier_post(struct mangle *mangle, const char *qualifiers)
{
  assert(mangle != NULL);
  assert(qualifiers != NULL);
  for (int i = 0; qualifiers[i] != '\0'; i++) {
    if (qualifiers[i] != 'R' && qualifiers[i] != 'O')
      append_space(mangle);
    if (qualifiers[i] == 'r')
      append(mangle, "restrict");
    else if (qualifiers[i] == 'V')
      append(mangle, "volatile");
    else if (qualifiers[i] == 'K')
      append(mangle, "const");
    else if (qualifiers[i] == 'R')
      append(mangle, "&");
    else if (qualifiers[i] == 'O')
      append(mangle, "&&");
  }
}

static void _extended_qualifier(struct mangle *mangle)
{
  /* <extended-qualifier> ::= ( U <source-name> <template-arg>* )+ <type>
   */
  assert(mangle != NULL);
  if (match(mangle, "U")) {
    /* find the end of extended-qualifiers */
    #define MAX_EXTQ  10
    char *base = current_position(mangle);
    const char *mpos_stack[MAX_EXTQ];
    int count = 0;
    do {
      mpos_stack[count++] = mangle->mpos;
      _source_name(mangle);
      _template_args(mangle);
    } while (count < MAX_EXTQ && mangle->valid && match(mangle, "U"));

    *base = '\0'; /* restore state */
    _type(mangle);

    const char *mpos_save = mangle->mpos;
    for (int i = count - 1; i >= 0; i--) {
      mangle->mpos = mpos_stack[i];
      append_space(mangle);
      _source_name(mangle);
      add_substitution(mangle, base, 0);
    }
    mangle->mpos = mpos_save;
  }
}

static void _template_args(struct mangle *mangle)
{
  /* <template->args> ::= I <template-arg>* E

     <template-arg> ::= <type>
                        X <expression> E      # expression (not yet handled)
                        <expr-primary>        # simple expressions (not yet handled)
                        J <template-arg>* E   # argument pack (not yet handled)
  */
  assert(mangle != NULL);
  if (match(mangle, "I")) {
    size_t tpl_base = mangle->tpl_subst_count;
    append(mangle, "<");
    int count = 0;
    while (mangle->valid && !match(mangle, "E")) {
      if (count++ > 0)
        append(mangle, ",");
      char *mark = current_position(mangle);
      _type(mangle);
      add_substitution(mangle, mark, 1);
    }
    append(mangle, ">");
    /* forget any previous template parameters, move new parameters up */
    if (tpl_base > 0) {
      free_tpl_subst(mangle, tpl_base);
      for (size_t i = tpl_base; i < mangle->tpl_subst_count; i++) {
        mangle->tpl_subst[i - tpl_base] = mangle->tpl_subst[i];
        mangle->tpl_subst[i] = 0;
      }
      mangle->tpl_subst_count -= tpl_base;
    }
  }
}

static void _discriminator(struct mangle *mangle)
{
  /* <discriminator> ::= _ <digit> _
                         _ _ <digit> <digit>+ _
  */
  assert(mangle != NULL);
  if (match(mangle, "_")) {
    if (match(mangle, "_")) {
      while (isdigit(*mangle->mpos))
        mangle->mpos += 1;      /* skip (ignore) all following digits */
      expect(mangle, "_");
    } else {
      mangle->mpos += 1;        /* skip (ignore) single digit discriminator */
    }
  }
}

static void _source_name(struct mangle *mangle)
{
  /* <source-name> ::= <number> <character>+      #string with length prefix
   */
  assert(mangle != NULL);
  if (mangle->valid) {
    if (!isdigit(*mangle->mpos)) {
      mangle->valid = 0;
      return;
    }
    int count = (int)strtol(mangle->mpos, (char**)&mangle->mpos, 10);
    if ((int)strlen(mangle->mpos) < count) {
      mangle->valid = 0;
      return;
    }
    char *tmpname = _alloca(count + 1);
    memcpy(tmpname, mangle->mpos, count);
    tmpname[count] = '\0';
    append(mangle, tmpname);
    mangle->mpos += count;
  }
}

static void _unqualified_name(struct mangle *mangle)
{
  /* <unqualified-name> ::= <operator-name> [<abi-tags>]
                            <ctor-dtor-name>
                            <source-name>
                            L <source-name> <discriminator> # <local-source-name>
                            DC <source-name>+ E   # structured binding declaration
                            Ut [ <number> ] _     # <unnamed-type-name>
                            Ul <lambda-sig> E [ <nonnegative number> ] _  # <closure-type-name>
  */
  assert(mangle != NULL);
  if (mangle->valid) {
    if (match(mangle, "DC")) {
      while (isdigit(*mangle->mpos))
        _source_name(mangle);
      expect(mangle, "E");
    } else if (peek(mangle, "Ut")) {
      _unnamed_type_name(mangle);
    } else if (isdigit(*mangle->mpos)) {
      _source_name(mangle);
    } else if (match(mangle, "L")) {
      _source_name(mangle);
      _discriminator(mangle);
    } else if (peek(mangle, "C1") || peek(mangle, "C2") || peek(mangle, "C3")
               || peek(mangle, "CI1") || peek(mangle, "CI2")
               || peek(mangle, "D0") || peek(mangle, "D1") || peek(mangle, "D2")) {
      _ctor_dtor_name(mangle);
    } else if (is_operator(mangle) >= 0) {
      _operator(mangle);
    } else {
      mangle->valid = 0;
    }
  }
}

static void _decltype(struct mangle *mangle)
{
  /* <decltype>  ::= Dt <expression> E  # decltype of an id-expression or class member access
                     DT <expression> E  # decltype of an expression

   */
  assert(mangle != NULL);
  if (!match(mangle, "Dt"))
    expect(mangle, "DT");
  if (mangle->valid) {
    //??? not yet implemented
    expect(mangle, "E");
  }
}

static void _function_type(struct mangle *mangle)
{
  /* <function-type> ::= F [Y] <return-type> <parameter-type>* [<ref-qualifier>] E
   */
  assert(mangle != NULL);
  if (expect(mangle, "F")) {
    _type(mangle);

    /* get the parameter list */
    char *plist = current_position(mangle);
    mangle->func_nest += 1;
    assert(mangle->func_nest < MAX_FUNC_NESTING);
    append(mangle, "(");
    int count = 0;
    while (mangle->valid && !peek(mangle, "E")) {
      char *mark = current_position(mangle);
      mangle->parameter_base[mangle->func_nest] = mark;
      if (count > 0)
        append(mangle, ",");
      _type(mangle);
      /* special case for functions without parameters: erase "void" */
      if (count == 0 && strcmp(mark, "void") == 0 && peek(mangle, "E"))
        *mark = '\0';
      count++;
    }
    append(mangle, ")");
    expect(mangle, "E");
    mangle->func_nest -= 1;

    /* move the parameter list into position */
    if (mangle->parameter_base[mangle->func_nest] != 0) {
      size_t len = strlen(plist);
      char *buffer = _alloca((len + 1) * sizeof(char));
      strcpy(buffer, plist);
      *plist = '\0';
      char *pos = insertion_point(mangle, mangle->parameter_base[mangle->func_nest]);
      insert(mangle, pos, buffer);
    }
  }
}

static void _pointer_to_member_type(struct mangle *mangle)
{
  /* <pointer-to-member-type> ::= M <(class) type> <(member) type>
   */
  assert(mangle != NULL);
  if (expect(mangle, "M")) {
    char *mark = current_position(mangle);
    /* class type, copy to local buffer because it must be moved relative to
       the member type */
    _type(mangle);
    size_t len = strlen(mark);
    char *classtype = _alloca((len + 10) * sizeof(char));  /* add some space, because of characters concatenated */
    strcpy(classtype, mark);
    strcat(classtype, "::*");
    *mark = '\0'; /* resture plain string */
    /* member type */
    _type(mangle);  /* member type */
    /* check for parentheses (function pointer) */
    char *p = insertion_point(mangle, mark);
    assert(p != NULL);
    if (*p == '(') {
      insert(mangle, p, " ()");
      p += 2;
    } else {
      insert(mangle, p, " ");
      p += 1;
    }
    insert(mangle, p, classtype);
    add_substitution(mangle, mark, 0);
  }
}

static void _array(struct mangle *mangle)
{
  /* <array-type> ::= A [ <number> ] _ <type>   # right-to-left associative
   */
  assert(mangle != NULL);
  if (expect(mangle, "A")) {
    /* collect & skip the array specifications (without parsing them) */
    #define MAX_ARRAYDIM  10
    const char *mpos_stack[MAX_ARRAYDIM];
    int count = 0;
    do {
      mpos_stack[count++] = mangle->mpos;
      while (*mangle->mpos != '_') {
        if (on_sentinel(mangle))
          mangle->valid = 0;
        mangle->mpos += 1;
      }
      expect(mangle, "_");
    } while (count < MAX_ARRAYDIM && match(mangle, "A"));

    char *mark = current_position(mangle);
    _type(mangle);  /* type of the array elements */
    if (!mangle->valid)
      return;

    const char *mpos_save = mangle->mpos;
    char *insert_pos = current_position(mangle);
    for (int i = count - 1; i >= 0; i--) {
      mangle->mpos = mpos_stack[i];
      char field[40];
      if (isdigit(*mangle->mpos))
        sprintf(field, "[%lu]", strtoul(mangle->mpos, NULL, 10));
      else
        strcpy(field, "[]");
      insert(mangle, insert_pos, field);
      add_substitution(mangle, mark, 0);
    }
    mangle->mpos = mpos_save;
  }
}

/** is_abbreviation() - returns the index of an operator record if the current
 *  position points to the code for a predefined substitution; or -1 if it does
 *  not point to a substitution.
 */
static int is_abbreviation(struct mangle *mangle)
{
  assert(mangle != NULL);
  if (strlen(mangle->mpos) < 2)
    return -1;
  for (int i = 0; i < sizearray(abbreviations); i++) {
    assert(strlen(abbreviations[i].abbrev) == 2);
    if (strncmp(mangle->mpos, abbreviations[i].abbrev, 2) == 0)
      return i;
  }
  return -1;
}

static void _substitution(struct mangle *mangle)
{
  /* <substitution> ::= S <seq-id> _
                        S_
   */
  assert(mangle != NULL);
  if (expect(mangle, "S")) {
    size_t index = 0;
    if (*mangle->mpos != '_') {
      while (*mangle->mpos != '_' && !on_sentinel(mangle)) {
        int digit;
        if (isdigit(*mangle->mpos)) {
          digit = *mangle->mpos - '0';
        } else if (isupper(*mangle->mpos)) {
          digit = *mangle->mpos - 'A' + 10;
        } else {
          mangle->valid = 0;
          return;
        }
        index = index * 36 + digit;
        mangle->mpos += 1;
      }
      index += 1;
    }
    expect(mangle, "_");
    if (index >= mangle->subst_count) {
      mangle->valid = 0;
      return;
    }
    assert(mangle->substitions[index] != NULL);
    append(mangle, mangle->substitions[index]);
  }
}

static void _template_param(struct mangle *mangle)
{
  /* <template-param> ::= T_ # first template parameter
                          T <parameter-2 non-negative number> _
   */
  assert(mangle != NULL);
  if (expect(mangle, "T")) {
    size_t index = 0;
    if (*mangle->mpos != '_')
      index = (int)strtol(mangle->mpos, (char**)&mangle->mpos, 10) + 1;
    expect(mangle, "_");
    if (index >= mangle->tpl_subst_count) {
      mangle->valid = 0;
      return;
    }
    assert(mangle->tpl_subst[index] != NULL);
    append(mangle, mangle->tpl_subst[index]);
    /* a template expansion is added as a substitution */
    add_substitution(mangle, mangle->tpl_subst[index], 0);
  }
}

static void _unnamed_type_name(struct mangle *mangle)
{
  /* <unnamed-type-name> ::= Ut [ <number> ] _
   */
  assert(mangle != NULL);
  if (expect(mangle, "Ut")) {
    /* ignore the sequence number */
    while (isdigit(*mangle->mpos))
      mangle->mpos += 1;
    expect(mangle, "_");
    append(mangle, "{unnamed type}");
  }
}

static void _local_name(struct mangle *mangle)
{
  /* <local-name> ::= Z <function-encoding> E <(entity) name> [<discriminator>]
                      Z <function-encoding> E s [<discriminator>]
   */
  assert(mangle != NULL);
  if (expect(mangle, "Z")) {
    mangle->func_nest += 1;
    _function_encoding(mangle);
    mangle->func_nest -= 1;
    append(mangle, "::");

    expect(mangle, "E");
    if (match(mangle, "s"))
      append(mangle, "{string-literal}");
    else
      _name(mangle);

    _discriminator(mangle);
  }
}

static void _ctor_dtor_name(struct mangle *mangle)
{
  /* <ctor-dtor-name> ::= C1            # complete object constructor
                          C2            # base object constructor
                          C3            # complete object allocating constructor
                          CI1 <base class type> # complete object inheriting constructor
                          CI2 <base class type> # base object inheriting constructor
                          D0            # deleting destructor
                          D1            # complete object destructor
                          D2            # base object destructor
   */
  assert(mangle != NULL);
  if (mangle->valid) {
    const char *tail = mangle->plain + strlen(mangle->plain);
    if (tail > mangle->plain + 2 && *(tail - 1) == ':' && *(tail - 2) == ':')
      tail -= 2;
    const char *head = tail;
    while (head != mangle->plain && (isalpha(*(head - 1)) || isdigit(*(head - 1)) || *(head - 1) == '_'))
      head -= 1;  /* find start of class name */
    if (head == tail) {
      mangle->valid = 0;
      return;
    }
    size_t len = tail - head;
    char *cname = _alloca((len + 1) * sizeof(char));
    memcpy(cname, head, len);
    cname[len] = '\0';
    if (*tail != ':')
      append(mangle, "::");
    assert(*mangle->mpos == 'C' || *mangle->mpos == 'D');
    if (*mangle->mpos == 'D')
    append(mangle, "~");
    append(mangle, cname);
    mangle->mpos += 1;  /* skip 'C' or 'D' */
    if (*mangle->mpos == 'I')
      mangle->mpos += 1;
    assert(isdigit(*mangle->mpos));
    mangle->mpos += 1;  /* skip type id */
  }
}

/** is_operator() - returns the index of an operator record if the current
 *  position points to the code for an (overloaded) operator; or -1 if it does
 *  not point to an operator code.
 */
static int is_operator(struct mangle *mangle)
{
  assert(mangle != NULL);
  if (strlen(mangle->mpos) < 2)
    return -1;
  for (int i = 0; i < sizearray(operators); i++) {
    assert(strlen(operators[i].abbrev) == 2);
    if (strncmp(mangle->mpos, operators[i].abbrev, 2) == 0)
      return i;
  }
  return -1;
}

static void _operator(struct mangle *mangle)
{
  assert(mangle != NULL);
  if (mangle->valid) {
    if (strlen(mangle->mpos) < 2) {
      mangle->valid = 0;
      return;
    }
    int i = is_operator(mangle);
    if (i < 0) {
      mangle->valid = 0;
      return;
    }
    mangle->mpos += 2;
    append_space(mangle);
    append(mangle, "operator");
    if (i == 0) {
      /* special case for typecast operator */
      append(mangle, " ");
      _type(mangle);
      mangle->is_typecast_op = 1;
    } else {
      if (isalpha(operators[i].name[0]))
        append(mangle, " ");
      append(mangle, operators[i].name);
    }
    //??? handle abi-tags
    //    <abi-tag> := B <source-name> # right-to-left associative
  }
}

static void _expr_primary(struct mangle *mangle)
{
  /* <expr-primary> ::= L <type> <number> E                              # integer literal
                        L <type> <float> E                               # floating literal
                        L <string type> E                                # string literal
                        L <nullptr type> E                               # nullptr literal (i.e., "LDnE")
                        L <pointer type> 0 E                             # null pointer template argument
                        L <type> <(real) float> _ <(imaginary) float> E  # complex floating point literal (C 2000)
                        L _Z <encoding> E                                # external name
   */
  assert(mangle != NULL);
  if (expect(mangle, "L")) {
    char t = *mangle->mpos;
    char field[64];
    if (t == 's' || t == 'i' || t == 'l' || t == 'x') {
      mangle->mpos += 1;
      if (*mangle->mpos == 'n') {
        append(mangle, "-");
        mangle->mpos += 1;
      }
      get_number(mangle, field, sizearray(field), 0);
      append(mangle, field);
    } else if (t == 't' || t == 'j' || t == 'm' || t == 'y') {
      mangle->mpos += 1;
      get_number(mangle, field, sizearray(field), 0);
      append(mangle, field);
    } else if (t == 'f' || t == 'd' || t == 'e') {
      mangle->mpos += 1;
      get_number(mangle, field, sizearray(field), 1);
      if (t == 'f')
        append(mangle, "(float){");
      else if (t == 'd')
        append(mangle, "(double){");
      else
        append(mangle, "(long double){");
      append(mangle, field);
      append(mangle, "}");
    } else if (t == 'c' || t == 'a' || t == 'h') {
      mangle->mpos += 1;
      get_number(mangle, field, sizearray(field), 0);
      if (t == 'c')
        append(mangle, "(char)");
      else if (t == 'a')
        append(mangle, "(signed char)");
      else if (t == 'h')
        append(mangle, "(unsigned char)");
      append(mangle, field);
    } else if (t == 'A') {
      mangle->mpos += 1;
      long len = strtol(mangle->mpos, (char**)&mangle->mpos, 10);
      expect(mangle, "_");
      if (match(mangle, "Kc"))
        append(mangle, "\"");
      else if (match(mangle, "Kw"))
        append(mangle, "L\"");
      for (long i = 0; i < len; i++)
        append(mangle, "?");
      append(mangle, "\"");
    } else if (match(mangle, "Dn")) {
      append(mangle, "nullptr");
    } else {
      mangle->valid = 0;
      return;
    }
    expect(mangle, "E");
  }
}

static void _nested_name(struct mangle *mangle)
{
  /* <nested-name> ::= N [<CV-qualifiers>] [<ref-qualifier>] <prefix> <name-param>* E

     <prefix> ::= <unqualified-name>        # global class or namespace
              ::= <decltype>                # decltype qualifier
              ::= <template-param>          # template parameter (T_, T0_, etc.)
              ::= <substitution>

     <name-param> ::= <unqualified-name>    # nested class or namespace (left-recursion!)
                  ::= <template-arg>*       # <template-prefix> class template specialization
                  ::= M                     # <closure-prefix> initializer of a variable or data member
   */
  assert(mangle != NULL);
  if (expect(mangle, "N")) {
    mangle->nest += 1;

    /* <CV-qualifiers> and <ref-qualifier> (append at end) */
    char qualifiers[8];
    _qualifier_pre(mangle, qualifiers, sizearray(qualifiers), 1);

    char *mark = current_position(mangle);

    /* prefix */
    if (peek(mangle, "Dt") || peek(mangle, "DT")) {
      _decltype(mangle);
      add_substitution(mangle, mark, 0);
    } else if (is_abbreviation(mangle) >= 0) {
      int i = is_abbreviation(mangle);
      assert(i >= 0 && i < sizearray(abbreviations));
      assert(strlen(abbreviations[i].abbrev) == 2);
      mangle->mpos += 2;
      append(mangle, abbreviations[i].name);
    } else if (peek(mangle, "S") && (isdigit(mangle->mpos[1]) || isupper(mangle->mpos[1]) || mangle->mpos[1]== '_')) {
      _substitution(mangle);
    } else if (peek(mangle, "T") && (isdigit(mangle->mpos[1]) || mangle->mpos[1] == '_')) {
      _template_param(mangle);
    } else {
      _unqualified_name(mangle);
      add_substitution(mangle, mark, 0);
    }
    /* at least one name should follow, so separator can be appended */
    if (match(mangle, "E")) {
      mangle->valid = 0;
      return;
    }

    int sentinel = 0;
    do {
      if (peek(mangle, "M")) {
        continue; /* closure type, ignore */
      } else if (peek(mangle, "I")) {
        _template_args(mangle);
      } else {
        append(mangle, "::");
        _unqualified_name(mangle);
      }
      sentinel = match(mangle, "E");
      if (!sentinel || mangle->nest > 1)
        add_substitution(mangle, mark, 0);  /* don't add function name at global level */
    } while (mangle->valid && !sentinel);

    if (mangle->nest > 1)
      _qualifier_post(mangle, qualifiers);
    else
      strcpy(mangle->qualifiers, qualifiers);  /* special case: appended after
                                                  handling function parameters (if any) */
    mangle->nest -= 1;
  }
}

static void _name(struct mangle *mangle)
{
  /* <name> := N <nested-name> E
               Z <local-name> E (<name> | s) [ (_ <number> | _ _ <number> _ )
               <unscoped-name> <template-arg>*

     <unscoped-name> := St <unqualified-name>   #::std::
                        <subtitution>           # S <base-36-number>
                        <unqualified-name>

     <unqualified-name> := <operator-name> <abi-tag>*
                           <ctor-dtor-name>
                           <source-name>        # <number> <text>
                           DC <source-name>+ E
                           Ut <unnamed-type-name> _

     <abi-tag> := B <source-name> # right-to-left associative
   */
  assert(mangle != NULL);
  char *mark = current_position(mangle);
  int is_unscoped = 1;
  if (mangle->valid) {
    if (peek(mangle, "N")) {
      _nested_name(mangle);
      is_unscoped = 0;
    } else if (peek(mangle, "Z")) {
      _local_name(mangle);
      is_unscoped = 0;
    } else if (is_abbreviation(mangle) == 0) {
      assert(strlen(abbreviations[0].abbrev) == 2);
      mangle->mpos += 2;
      append(mangle, abbreviations[0].name);
      append(mangle, "::");
      _unqualified_name(mangle);
    } else if (peek(mangle, "S") && (isdigit(mangle->mpos[1]) || isupper(mangle->mpos[1]) || mangle->mpos[1] == '_')) {
      _substitution(mangle);
    } else if (is_operator(mangle) >= 0) {
      _operator(mangle);
      //??? handle abi-tags (right-to-left)
    } else if (peek(mangle, "C1") || peek(mangle, "C2") || peek(mangle, "C3")
               || peek(mangle, "CI1") || peek(mangle, "CI2")
               || peek(mangle, "D0") || peek(mangle, "D1") || peek(mangle, "D2")) {
      _ctor_dtor_name(mangle);
    } else if (isdigit(*mangle->mpos)) {
      _source_name(mangle);
    } else if (match(mangle, "L")) {
      _source_name(mangle);
      _discriminator(mangle);
    } else if (match(mangle, "DC")) {
      while (isdigit(*mangle->mpos))
        _source_name(mangle);
      expect(mangle, "E");
    } else if (peek(mangle, "Ut")) {
      _unnamed_type_name(mangle);
    } else {
      mangle->valid = 0;
    }
  }

  if (is_unscoped && peek(mangle, "I")) {
    add_substitution(mangle, mark, 0);
    _template_args(mangle);
  }
}

/** is_stdtype() - returns the index of an operator record if the current
 *  position points to the code for a standard type; or -1 if it does not point
 *  to a standard type.
 */
static int is_builtin_type(struct mangle *mangle)
{
  assert(mangle != NULL);
  size_t remaining = strlen(mangle->mpos);
  if (remaining < 1)
    return -1;
  for (int i = 0; i < sizearray(types); i++) {
    size_t len = strlen(types[i].abbrev);
    if (len <= remaining && strncmp(mangle->mpos, types[i].abbrev, len) == 0)
      return i;
  }
  return -1;
}

static void _type(struct mangle *mangle)
{
  /* <type> ::= <builtin-type>
                <cv-qualifier>+ <type>  # qualifier is appended at the end
                <function-type>
                <class-enum-type>
                <array-type>
                <pointer-to-member-type>
                <source-name> <template-arg>*
                <template-param> <template-arg>*  # template parameter (T_, T0_, etc.)
                <substitution> <template-arg>*    # S_, S0_, etc.
                <decltype>
                <nested-name>
                <local-name>
                Dp <type>         # pack expansion
                P <type>          # pointer
                R <type>          # l-value reference
                O <type>          # r-value reference (C++11)
                C <type>          # complex pair (C99)
                G <type>          # imaginary (C99)
                L <type> <value>  # literal

     <vector-type> ::= Dv <number> _ <type>
                   ::= Dv _ <expression> _ <type>

     <cv-qualifier> ::= U <source-name> <template-arg>* # vendor extended type qualifier
                        r    # restrict (C99)
                        V    # volatile
                        K    # const
  */
  assert(mangle != NULL);
  if (mangle->valid) {
    char *mark = current_position(mangle);
    if (is_builtin_type(mangle) >= 0) {
      int i = is_builtin_type(mangle);
      assert(i >= 0 && i < sizearray(types));
      mangle->mpos += strlen(types[i].abbrev);
      append(mangle, types[i].name);
    } else if (peek(mangle, "r") || peek(mangle, "V") || peek(mangle, "K")) {
      char qualifiers[8];
      _qualifier_pre(mangle, qualifiers, sizearray(qualifiers), 0);
      _type(mangle);
      _qualifier_post(mangle, qualifiers);
      add_substitution(mangle, mark, 0);
    } else if (peek(mangle, "U")) {
      _extended_qualifier(mangle);
    } else if (peek(mangle, "F")) {
      _function_type(mangle);
      add_substitution(mangle, mark, 0);
    } else if (peek(mangle, "A")) {
      _array(mangle);
    } else if (match(mangle, "P")) {
      _type(mangle);
      char *p = insertion_point(mangle, mark);
      assert(p != NULL);
      if (*p == '(' || *p == '[')
        insert(mangle, p, "(*)");
      else
        insert(mangle, p, "*");
      add_substitution(mangle, mark, 0);
    } else if (match(mangle, "R")) {
      _type(mangle);
      char *p = insertion_point(mangle, mark);
      assert(p != NULL);
      if (*p == '(' || *p == '[')
        insert(mangle, p, "(&)");
      else
        insert(mangle, p, "&");
      add_substitution(mangle, mark, 0);
    } else if (match(mangle, "O")) {
      _type(mangle);
      append(mangle, "&&");
      add_substitution(mangle, mark, 0);
    } else if (is_abbreviation(mangle) >= 0) {
      int i = is_abbreviation(mangle);
      assert(i >= 0 && i < sizearray(abbreviations));
      assert(strlen(abbreviations[i].abbrev) == 2);
      mangle->mpos += 2;
      append(mangle, abbreviations[i].name);
      if (i == 0) {
        append(mangle, "::"); /* special case for std:: */
        _unqualified_name(mangle);
        add_substitution(mangle, mark, 0);
      }
      if (peek(mangle, "I")) {
        _template_args(mangle);
        add_substitution(mangle, mark, 0);
      }
    } else if (peek(mangle, "S") && (isdigit(mangle->mpos[1]) || isupper(mangle->mpos[1]) || mangle->mpos[1]== '_')) {
      _substitution(mangle);
      _template_args(mangle);
    } else if (peek(mangle, "T") && (isdigit(mangle->mpos[1]) || mangle->mpos[1] == '_')) {
      _template_param(mangle);
      _template_args(mangle);
    } else if (peek(mangle, "N")) {
      _nested_name(mangle);
    } else if (peek(mangle, "Z")) {
      _local_name(mangle);
    } else if (peek(mangle, "M")) {
      _pointer_to_member_type(mangle);
    } else if (peek(mangle, "L")) {
      _expr_primary(mangle);
    } else if (isdigit(*mangle->mpos) || (*mangle->mpos == 'u') && isdigit(*(mangle->mpos + 1))) {
      if (*mangle->mpos == 'u')
        mangle->mpos += 1;  /* ignore "vendor-extended" type (N.B. Itanium ABI uses upper-case 'U', but c++filt only accepts lower-case 'u') */
      _source_name(mangle);
      add_substitution(mangle, mark, 0);
      _template_args(mangle);
    } else {
      mangle->valid = 0;
      return;
    }
  }
}

static void _function_encoding(struct mangle *mangle)
{
  _name(mangle);

  if (on_sentinel(mangle) || (mangle->nest > 0 && match(mangle, "E"))) {
    if (mangle->func_nest > 0)
      mangle->valid = 0;
    return;
  }
  if (strlen(mangle->plain) == 0) {
    mangle->valid = 0;
    return;
  }

  /* function parameter list
     list of types (absent for variables, at least one type for functions
     first type is the function return type, but it is only present when
     functions are template instantiations.
   */
  mangle->nest += 1;
  /* check whether a return type is present; save it but process it later */
  char *type_string = NULL;
  size_t type_ins_point = 0;
  if (mangle->plain[strlen(mangle->plain) - 1] == '>' && !mangle->is_typecast_op) {
    char *mark = current_position(mangle);
    _type(mangle);
    type_string = _alloca((strlen(mark) + 5) * sizeof(char));
    strcpy(type_string, mark);
    char *ipos = insertion_point(mangle, mark);
    type_ins_point = ipos - mark;
    *mark = '\0';
  }

  /* handle parameters */
  append(mangle, "(");
  int count = 0;
  while (!on_sentinel(mangle) && !(mangle->func_nest > 0 && peek(mangle, "E"))) {
    char *mark = current_position(mangle);
    mangle->parameter_base[mangle->func_nest] = mark;
    if (count > 0)
      append(mangle, ",");
    _type(mangle);
    /* special case for functions without parameters: erase "void" */
    if (count == 0 && strcmp(mark, "void") == 0
        && (on_sentinel(mangle) || (mangle->func_nest > 0 && peek(mangle, "E"))))
      *mark = '\0';
    count++;
  }
  mangle->nest -= 1;
  append(mangle, ")");
  if (mangle->nest == 0)
    _qualifier_post(mangle, mangle->qualifiers);

  /* prefix function type (saved earlier) */
  if (type_string != NULL) {
    assert(type_ins_point <= strlen(type_string));
    if (type_ins_point == strlen(type_string)) {
      strcat(type_string, " ");
    } else {
      /* split the buffer in two, append the last part (insert the first part) */
      append(mangle, type_string + type_ins_point);
      type_string[type_ins_point] = '\0';
    }
    insert(mangle, mangle->plain, type_string);
  }
}

static void _encoding(struct mangle *mangle)
{
  /* <encoding> ::= <name> [J]<type>* # type list is present for functions, absent for variables
                    TV <type>         # vtable
                    TT <type>         # vtable index
                    TI <type>         # typeinfo struct
                    TS <type>         # typeinfo name
  */
  assert(mangle != NULL);
  if (match(mangle, "TV")) {
    append(mangle, "vtable for ");
    _type(mangle);
  } else if (match(mangle, "TT")) {
    append(mangle, "vtable index for ");
    _type(mangle);
  } else if (match(mangle, "TI")) {
    append(mangle, "typeinfo for ");
    _type(mangle);
  } else if (match(mangle, "TS")) {
    append(mangle, "typeinfo name for ");
    _type(mangle);
  } else {
    _function_encoding(mangle);
  }
}

int demangle(char *plain, size_t size, const char *mangled)
{
  assert(plain != NULL);
  assert(size > 0);
  assert(mangled != NULL);

  /* <mangled-name> := _Z <encoding>
                       _Z <encoding> . <vendor-specific suffix>   #not currently handled
   */
  if (mangled[0] != '_' || mangled[1] != 'Z')
    return 0;

  struct mangle mangle;
  mangle.plain = plain;
  mangle.size = size;
  mangle.mangled = mangled;
  mangle.mpos = mangle.mangled + 2; /* skip "_Z" */

  memset(mangle.substitions, 0, MAX_SUBSTITUTIONS * sizeof(char *));
  mangle.subst_count = 0;
  memset(mangle.tpl_subst, 0, MAX_TEMPLATE_SUBST * sizeof(char*));
  mangle.tpl_subst_count = 0;
  memset(mangle.parameter_base, 0, MAX_FUNC_NESTING * sizeof(char*));
  mangle.func_nest = 0;
  memset(mangle.qualifiers, 0, sizeof mangle.qualifiers);

  mangle.valid = 1;
  mangle.nest = 0;
  mangle.is_typecast_op = 0;
  mangle.plain[0] = '\0';
  _encoding(&mangle);

  free_tpl_subst(&mangle, mangle.tpl_subst_count);
  for (size_t i = 0; i < mangle.subst_count; i++) {
    assert(mangle.substitions[i] != NULL);
    free((void*)mangle.substitions[i]);
  }
  return mangle.valid;
}

