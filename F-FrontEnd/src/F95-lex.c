/* 
 * $TSUKUBA_Release: Omni OpenMP Compiler 3 $
 * $TSUKUBA_Copyright:
 *  PLEASE DESCRIBE LICENSE AGREEMENT HERE
 *  $
 */
/**
 * \file F95-lex.c
 */

#include <stdint.h>

/* lexical analyzer, enable open mp.  */
int OMP_flag = FALSE;

/* lexical analyzer */
/* enable to parse  for progma.  */
int PRAGMA_flag = TRUE;

#define TOLOWER(x) (isupper((int)(x))) ? tolower((int)(x)) : (x)

#define ST_BUF_SIZE     65536
#define LINE_BUF_SIZE   256

int st_class;           /* token for classify statement */
int need_keyword = FALSE;
int need_type_len = FALSE;
int need_check_user_defined = TRUE; /* check the user defined dot id */

int may_generic_spec = FALSE;

int fixed_format_flag = FALSE; 
int fixed_line_len = 72;

/* enable  comment char, 'c' in free format.  */
int flag_force_c_comment = FALSE; /* does not set yet.  */

#define QUOTE   '\002'  /* special quote mark */

int function_apperable = TRUE;

struct keyword_token {
    char *k_name;
    int k_token;
};

#define IS_CONT_LINE(x) ((x)[5] != ' ' && (x)[5] != '0')

extern struct keyword_token dot_keywords[],keywords[];
extern struct keyword_token end_keywords[];

int exposed_comma,exposed_eql;
int paren_level;
char *bufptr;                   /* pointer running in st_buffer */
char st_buffer[ST_BUF_SIZE];    /* one statement buffer */
char st_buffer_org[ST_BUF_SIZE];        /* one statement buffer  */
char stn_cols[7];                       /* line number colums */
char line_buffer[LINE_BUF_SIZE];        /* pre_read line buffer */
char buffio[LINE_BUF_SIZE];

int prevline_is_inQuote = 0;
int prevline_is_inComment = FALSE;

expr st_name; /* statement name */

static int line_count = 0;
lineno_info *current_line;
int pre_read = 0;
lineno_info read_lineno;

int is_using_module = FALSE;

struct saved_file_state {
    FILE *save_fp;
    lineno_info *save_line;
    int save_pre_read;
    int is_using_module; /* TRUE if file is module file. */
    lineno_info save_lineno;
    char *save_buffer;
    char *save_stn_cols;
    /* when include by USE, must specify -force-free-format.  */
    int save_fixed_format_flag;
    int save_no_countup; /* reach ';', we do not count up the line number.  */
} file_state[N_NESTED_FILE];

int n_nested_file = 0;

/* input file table */
int     n_files = 0;
char    *file_names[MAX_N_FILES];

enum lex_state 
{
    LEX_NEW_STATEMENT = 0,
    LEX_FIRST_TOKEN,    
    LEX_OTHER_TOKEN,    
    LEX_OMP_TOKEN,
    LEX_RET_EOS 
};

int st_OMP_flag;

enum lex_state lexstate;
int st_PRAGMA_flag;

/* read_line return value */
#define ST_EOF  0
#define ST_INIT 1
#define ST_CONT 2
#define ST_ONE  ST_INIT


/* file position of initial line tail read last */
long last_initial_line_pos = 0;
/* file position of initial line tail read last save one */
long prelast_initial_line_pos = 0;

/* Keep offset for last line and before the last.
   In fixed format, for read prefetch read offset is miss count.
   Cause MC has miss seek and funny error like "missing initial line".
 */
static long last_offset[2];
static int last_ln_nos[2];
int last_ln_no = 0;

/* when read ';', no need for line count up.  */
static int no_countup = FALSE;

static int      read_initial_line _ANSI_ARGS_((void));
static int      classify_statement _ANSI_ARGS_((void));
static int      token _ANSI_ARGS_((void));
static int      is_not_keyword _ANSI_ARGS_((void));
static int      get_keyword _ANSI_ARGS_((struct keyword_token *ks));
static int      get_keyword_optional_blank _ANSI_ARGS_((int class));
static int      readline_free_format _ANSI_ARGS_((void));
static int      read_number _ANSI_ARGS_((void));
static int      read_identifier _ANSI_ARGS_((void));

static void     string_to_integer _ANSI_ARGS_((omllint_t *p, char *cp, int radix));
static double   convert_str_double _ANSI_ARGS_((char *s));

static int      read_initial_line _ANSI_ARGS_((void));
static int      read_fixed_format _ANSI_ARGS_((void));
static int      read_free_format _ANSI_ARGS_((void));
static int      readline_free_format _ANSI_ARGS_((void));
static int      readline_fixed_format _ANSI_ARGS_((void));
static int      is_PRAGMA_sentinel _ANSI_ARGS_((char **));
static int	    is_OMP_sentinel _ANSI_ARGS_((char **));
static int      find_last_ampersand _ANSI_ARGS_((char *buf,int *len));

static void     save_format_str _ANSI_ARGS_((void));




/* for free format.  */
/* pragma string setter. */
static void     set_pragma_str _ANSI_ARGS_((char *));
static void     append_pragma_str _ANSI_ARGS_((char *));


static void     restore_file(void);

static int      ScanFortranLine _ANSI_ARGS_((char *src, char *srcHead,
                                             char *dst, char *dstHead, char *dstMax,
                                             int *inQuotePtr, int *quoteCharPtr,
                                             int *inHollerithPtr, int *hollerithLenPtr, 
                                             char **newCurPtr, char **newDstPtr));

static void
debugOutStatement()
{
    int len = strlen(line_buffer);
    char *trimBuf = (char *)alloca(len + 1);
    char *nlcr = NULL;

    snprintf(trimBuf, len + 1, "%s", line_buffer);
    nlcr = strpbrk(trimBuf, "\r\n");
    if (nlcr != NULL) {
        *nlcr = '\0';
    }

    if (fixed_format_flag) {
        fprintf(debug_fp, "%6d:'%s'|\"%s\"\n",
                read_lineno.ln_no,
                stn_cols,
                trimBuf);
    } else { 
        fprintf(debug_fp, "%6d:\"%s\"\n",
                read_lineno.ln_no,
                trimBuf);
    }
    fflush(debug_fp);
}

void
initialize_lex()
{
  extern int mcLn_no;
  extern long mcStart;
  
  memset(last_ln_nos, 0, sizeof(last_ln_nos));

    /* set lineno info as default */
  if (mcLn_no == -1)
    read_lineno.ln_no = 0;
  else {
    if (fseek (source_file, mcStart, SEEK_SET) == -1) {
      error ("internal comiler error: cannot seek given start point in initialize_lex()");
      exit(-1);
    }
    read_lineno.ln_no = mcLn_no;
  }
    if(source_file_name != NULL)
        read_lineno.file_id = get_file_id(source_file_name);
    else
        read_lineno.file_id = get_file_id("<stdin>"); 

    lexstate = LEX_NEW_STATEMENT;
    exposed_comma = FALSE;
    exposed_eql = FALSE;
    paren_level = 0;
}

/* #define LEX_DEBUG */
static int yylast;

/* lexical analyer */
int
yylex()
{
    yylast = yylex0();
#ifdef LEX_DEBUG
    printf("%c[%d]",(yylast < ' ' || yylast >= 0xFF) ? ' ': yylast, yylast);
    fflush(stdout);
#endif
    return(yylast);
}

static int
yylex0()
{
    int t;
    static int tkn_cnt; 

    switch(lexstate){
    case LEX_NEW_STATEMENT:
    again:
        if(read_initial_line() == ST_EOF){
            if(n_nested_file > 0){
                restore_file();
                goto again;
            }
            return(ST_EOF);
        }
        
        /* set bufptr st_buffer */
        bufptr = st_buffer;
        if(st_OMP_flag){
            lexstate = LEX_OMP_TOKEN;
            return OMPKW_LINE;
        }
        if (st_PRAGMA_flag && !OMP_flag) {
            lexstate = LEX_OMP_TOKEN;
            return PRAGMA_HEAD;
        }
        tkn_cnt = 0;
        lexstate = LEX_FIRST_TOKEN;
        return(STATEMENT_LABEL_NO);

    case LEX_FIRST_TOKEN:
    first:
        tkn_cnt = 1;

        st_class = classify_statement();
        if (st_class == FORMAT) {
            save_format_str();
            lexstate = LEX_RET_EOS;
        } else if (st_class == EOS) {
            lexstate = LEX_NEW_STATEMENT;
        } else {
            lexstate = LEX_OTHER_TOKEN;
        }
        return(st_class);

    case LEX_OTHER_TOKEN:
        /* check 'if' '(' ... ')' */
        if((st_class == LOGIF || (st_class == ELSEIFTHEN)) /* elseif in 95? */
           && paren_level == 0 && tkn_cnt > 3)
          goto first;
        tkn_cnt++;
        /* check 'assign' ... 'to' */
        if(st_class == ASSIGN && tkn_cnt == 3
           && bufptr[0] == 't' && bufptr[1] == 'o'){
            bufptr += 2;
            return(KW_TO);
        }
	else if (st_class == DO && tkn_cnt == 3 &&
		 bufptr[0] == 'w' && bufptr[1] == 'h' &&
		 bufptr[2] == 'i' && bufptr[3] == 'l' &&
		 bufptr[4] == 'e'){
	  bufptr += 5;
	  return KW_WHILE;
	}
        t = token();
        if (t == FORMAT){
            /*
             * "format" shouldn't be here...
             */
            save_format_str();
            lexstate = LEX_RET_EOS;
        } else if (t == EOS) {
            lexstate = LEX_NEW_STATEMENT;
        }
        return(t);

    case LEX_RET_EOS:
        lexstate = LEX_NEW_STATEMENT;
        return(EOS);

    case LEX_OMP_TOKEN:
        if (!fixed_format_flag)
            append_pragma_str(st_buffer_org);
        lexstate = LEX_RET_EOS;
        return PRAGMA_SLINE;

    default:
        fatal("lexstate");
    }
    return UNKNOWN;
}

static void
save_format_str()
{
    char *fmt = strchr(st_buffer_org, '(');
    char *end = NULL;

    if (formatString != NULL) {
        free(formatString);
    }

    if (fmt == NULL) {
        error("illegal format statement \"%s\"\n", st_buffer_org);
        return;
    } else {
        end = strrchr(fmt, ')');
        if (end == NULL) {
            error("illegal format statement \"%s\"\n", st_buffer_org);
            return;
        }
        end++;
        *end = '\0';
    }
    formatString = strdup(fmt);
}



/* buffer for progma with pragma key and rest of line.  */
static char pragmaBuf[512];

static void
set_pragma_str(char *p)
{
  strcpy(pragmaBuf, p);
}

static void
append_pragma_str(char *s)
{
    if (pragmaString != NULL) {
        free(pragmaString);
    }
    strcat(pragmaBuf, s);
    pragmaString = strdup(pragmaBuf);
}

/* flush line */
static void
flush_line()
{
    lexstate = LEX_RET_EOS;
    need_keyword = FALSE;
    need_type_len = FALSE;
}

char *lex_get_line()
{
    char *s;

    s = strdup(bufptr);
    lexstate = LEX_RET_EOS;     /* force terminate */
    return(s);
}


void
yyerror(s) 
     char *s;
{ 
    error("%s",s);
}

char *lexline(n)
     int *n;
{
    *n = strlen(bufptr);
    return(bufptr);
}


static int
token()
{
    register char ch, *p;
    int t;
    
    while(isspace(*bufptr)) bufptr++;  /* skip white space */
    
    if(need_keyword == TRUE) {  /* require keyword */
        need_keyword = FALSE;
        t = get_keyword(keywords);
        if(t == SUBROUTINE || t == FUNCTION)
            set_function_disappear();
        if(t != UNKNOWN) return(t);
    }

    if(need_type_len == TRUE){  /* for type_length */
        need_type_len = FALSE;
        ch = *bufptr;
        if(ch == '\0') return EOS;
        if(ch >= '0' && ch <= '9'){
            t = 0;
            while(isdigit((int)*bufptr)) t = t*10 + *bufptr++ - '0';
            yylval.val = GEN_NODE(INT_CONSTANT, t);
            return CONSTANT;
        } else return *bufptr++;
    }

    switch(ch = *bufptr++) {
    case '\0':
        return(EOS);
    case QUOTE:
        for(p = buffio; (ch = *bufptr++) != QUOTE ;)
	    if (ch == '\0')
		break;
	    else
		*p++ = ch;
        *p = 0;
        yylval.val = GEN_NODE(STRING_CONSTANT,strdup(buffio));
        return(CONSTANT);       /* hollerith */
    case '=':
        if(*bufptr == '=') {    
            /* "==" */
            bufptr++;
            return (EQ);        
        } 
        if(*bufptr == '>') {
            /* "=>" */
            bufptr++;
            return (REF_OP);    
        } 
        return('=');
    case '(': 
        paren_level++; 
	/* or interface operator (/), (/=), or (//) */
	if (*bufptr == '/') {
	    char *save = ++bufptr; /* check 'interface operator (/)' ? */

	    while(isspace(*bufptr)) bufptr++;  /* skip white space */
	    if (*bufptr == ')')  { /* (/) in interface operator.  */
		bufptr = save - 1;
		return('(');
	    }
	    else if (*bufptr == '=') { /* (/=) in interface operator.  */
		if (*++bufptr == ')') {
		    bufptr = save - 1;
		    return '(';
		}
	    }
	    else if (*bufptr == '/') { /* (//) ? */
		if (*++bufptr == ')') {
		    bufptr = save - 1;
		    return '(';
		}
	    }
	    bufptr = save;		
	    return L_ARRAY_CONSTRUCTOR;
	} else {
	    char *save = bufptr; /* check  '(LEN=' or '(KIND=' */
	    int t;
	    int save_n = need_keyword;
	    int save_p = paren_level;
	    need_keyword = 1;
	    t = token();
	    if (t == KW_LEN) {
		while(isspace(*bufptr)) bufptr++;  /* skip white space */
		if (*bufptr++ == '=')
		    return SET_LEN;
	    } else if (t == KW_KIND) {
		while(isspace(*bufptr)) bufptr++;  /* skip white space */
		if (*bufptr++ == '=')
		    return SET_KIND;
	    }
	    need_keyword = save_n;
	    bufptr = save;
	    paren_level = save_p;
	}
        return('(');
    case ')': 
        paren_level--; 
        return(')');
    case '+': 
    case '-': 
    case ',': 
    case '$': 
    case '|': 
    case '%':
    case '_': /* id should not has '_' in top.  */
        return(ch);

    case ':': 
        if(*bufptr == ':'){
            bufptr++;
            return(COL2);
        }
        return(':');

    case '*':
        if(*bufptr == '*') {
            bufptr++;
            return(POWER);
        }
        return('*');
    case '/':
        if(*bufptr == '/') {
            bufptr++;
            return(CONCAT);
        } 
        if(*bufptr == '=') {
            bufptr++;
            return(NE);
        } 
        if (*bufptr == ')') {
            bufptr++;
            paren_level--; 
            { /* check 'interface operator (/)' ? */
                char *save = bufptr;
                bufptr -= 3;
                while(isspace(*bufptr)) bufptr--;  /* skip white space */
                if (*bufptr == '(') { /* (/) in interface operator.  */
                    bufptr = save - 1;
                    return '/';
                }
                bufptr = save;
            }
            return R_ARRAY_CONSTRUCTOR;
        }
        return('/');
    case '.':
        if(isdigit((int)*bufptr)) goto number;
        t = get_keyword(dot_keywords);
        if (t != UNKNOWN)
            return t;
        else if (need_check_user_defined) {
            char user_defined[33];
            SYMBOL s;
            int i;

            user_defined[0] = '.';
            user_defined[32] = '\0';

            while(isspace(*bufptr))
                bufptr++;

            for (i = 1; i < 32; i++) {
                if (*bufptr == '\0')
                    break;
            
                else if (*bufptr == '.') {
                    user_defined[i++] = *bufptr++;
                    user_defined[i] = '\0';
                    break;
                }
                else if (isspace(*bufptr)) {
                    while(isspace(*bufptr))
                        bufptr++;
                    if (*bufptr == '.') {
                        user_defined[i++] = *bufptr++;
                        user_defined[i] = '\0';
                        break;
                    }
                }
                else
                    user_defined[i] = *bufptr++;
            }
#if 0
            s = find_symbol_without_allocate (user_defined);
#endif
            s = find_symbol (user_defined);
            if (s == NULL)
                return '.';
            else {
                yylval.val = GEN_NODE(IDENT, s);
                return USER_DEFINED_OP;
            }
        } else
	    return '.';
    case '>':
        if(*bufptr == '='){
            bufptr++;
            return GE;
        } 
        return(GT);
    case '<':
        if(*bufptr == '='){
            bufptr++;
            return(LE);
        } 
        return(LT);

#ifdef not  /* ! is used for comment line */
    case '!':
        if(*bufptr == '='){
            bufptr++;
            return(NE);
        } 
        return(NOT);
#endif    

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    number:             /* reading number */
        bufptr--;               /* back */
        return read_number();
            
    case '[':
    case ']':
	return ch;

    default:
        if(isalpha(ch) || ch == '_') {
            bufptr--;           /* back */
            return read_identifier();
        } else
            error("bad char %c(0x%x)",ch,ch&0xFF);
        return UNKNOWN;
    }
}
              
static int 
read_identifier()
{
    int tkn_len;
    char *p,ch;

    p = buffio;
    /* id should not has '_' in top.  */
    for(tkn_len = 0;
        isalpha((int)*bufptr)
	    || isdigit((int)*bufptr)
	    || ((tkn_len >= 1) && (*bufptr == '_'));
        p++, bufptr++){
        *p = *bufptr;
        if(tkn_len++ > MAX_NAME_LEN && is_using_module == FALSE){
            error("name is too long");
        }
    }
    *p = 0;             /* termination */
    
#ifdef not
    if(strncmp(buffio,"function",8) == 0 && isalpha((int)buffio[8]) &&
               bufptr[0] == '(' && (bufptr[1] == ')' || isalpha((int)bufptr[1]))){
        /* back */
        bufptr -= (tkn_len - 8);
        return(FUNCTION);
    }
#endif

    /* ad hoc. read a head.  */
    /* caution!! if var name is "funciton", then miss parse here!!  */

    if (function_apperable == TRUE && strncmp (buffio, "function", 8) == 0) {
        char *save;

        if (fixed_format_flag) {
            bufptr -= strlen (buffio) - 8; /* put back for function-name-id.  */
        }
        save = bufptr;
        /* check the id is "function" and  */
        while (isspace(*bufptr))
            bufptr++;  /* skip white space */
        if (isalpha(*bufptr)) /* check the 'functio-name-id' */
            bufptr++;
        while (isalpha(*bufptr) || isdigit(*bufptr) || *bufptr == '_')
            bufptr++;
        while (isspace(*bufptr)) /* skip white space */
            bufptr++;  
        if (*bufptr == '(') { /* check '('  */
            bufptr++;
            while  (*bufptr) {
                if (*bufptr && *bufptr == ')') { /* anc check ')' */
                    bufptr = save; /* restore.  */
                    set_function_disappear();
                    return FUNCTION;
                }
                bufptr++;
            }
        }
        bufptr = save;  /* restore.  */
    }

    if(tkn_len == 1 && *bufptr == QUOTE){
        omllint_t v = 0;
        int radix = 0;
        /* x'hhhhh' constant */
        switch(buffio[0]) {
        case 'z': 
        case 'x':  radix = 16; break;
        case 'o':  radix = 8; break;
        case 'b': radix = 2; break;
        default: error("bad bit id");
        }
        tkn_len = 0;
        for(p = buffio, bufptr++; 
            (ch = *bufptr++) != QUOTE; *p++ = ch)
            if(tkn_len++ > 32) {
                error("too long constant");
                break;
            }
        *p = 0;         /* termination */
        if (radix == 0) {
            error("can't determine radix");
        }
        string_to_integer(&v, buffio, radix);
        yylval.val = make_int_enode(v);
        return(CONSTANT);
    } 
#ifdef YYDEBUG
    if (yydebug)
      fprintf (stderr, "read_identifier/(%s)\n", buffio);
#endif
    if (may_generic_spec &&
	((strcmp(buffio, "operator") == 0)
	 || (strcmp(buffio, "assignment") == 0))) {
	char *save = bufptr;
	int t;
	int save_n = need_keyword;
	int save_p = paren_level;
	need_keyword = 1;
	while (isspace(*bufptr)) /* skip white space */
	    bufptr++;
	if (*bufptr != '(') {
	    need_keyword = save_n;
	    bufptr = save;
	    paren_level = save_p;
	    goto returnId;
	} else
	    bufptr++;
	t = token();
	while (isspace(*bufptr)) /* skip white space */
	    bufptr++;
	if (*bufptr != ')') {
	    need_keyword = save_n;
	    bufptr = save;
	    paren_level = save_p;
	    goto returnId;
	}
	bufptr++;
    /* need to change this for compile phase in Front?  */
    if(t != USER_DEFINED_OP) {
        enum expr_code code;
        switch (t) {
        case '=' :    code = F95_ASSIGNOP; break;
        case '.' :    code = F95_DOTOP; break;
        case POWER :  code = F95_POWEOP; break;
        case '*' :    code = F95_MULOP; break;
        case '/' :    code = F95_DIVOP; break;
        case '+' :    code = F95_PLUSOP; break;
        case '-' :    code = F95_MINUSOP; break;
        case EQ :     code = F95_EQOP; break;
        case NE :     code = F95_NEOP; break;
        case LT :     code = F95_LTOP; break;
        case LE :     code = F95_LEOP; break;
        case GE :     code = F95_GEOP; break;
        case GT :     code = F95_GTOP; break;
        case NOT :    code = F95_NOTOP; break;
        case AND :    code = F95_ANDOP; break;
        case OR :     code = F95_OROP; break;
        case EQV :    code = F95_EQVOP; break;
        case NEQV :   code = F95_NEQVOP; break;
        case CONCAT : code = F95_CONCATOP; break;
        default :
          error("sytax error.");
          break;
        }
        yylval.val = list1(F95_GENERIC_SPEC, list0(code));
    } else {
        yylval.val = list1(F95_USER_DEFINED, yylval.val);
    }
    return GENERIC_SPEC;
    }

returnId:
    yylval.val = GEN_NODE(IDENT, find_symbol(buffio));
    return(IDENTIFIER);
}
        
static int
read_number()
{
    char *p,ch;

    int have_dot;
    int have_exp;
    int have_dbl;

    have_dot = FALSE;
    have_exp = FALSE;
    have_dbl = FALSE;

    p = buffio;
    while((ch = *bufptr) != '\0'){
        if(ch == '.'){
            if(have_dot) break; 
            else if(isalpha((int)bufptr[1]) && 
                    isalpha((int)bufptr[2])) 
                break;
            have_dot = TRUE;
        } else if(ch == 'd'|| ch =='e'){
            if(bufptr[1] == '+'||bufptr[1] =='-'){
                if(isdigit((int)bufptr[2])){
                    *p++ = ch;
                    bufptr++;
                    *p++ = *bufptr++;
                } else  break;
            } else if(isdigit((int)bufptr[1])) {
                *p++ = ch;
                bufptr++;
            } else break;
            have_exp = TRUE;
            if(ch == 'd') have_dbl = TRUE;
            while(isdigit((int)*bufptr)) *p++ = *bufptr++;
            break;
        } else if(!isdigit((int)ch))
            break;
        *p++ = ch;
        bufptr++;
    }
    *p = '\0';

    if (have_dbl || have_dot || have_exp) { 
        yylval.val = make_float_enode(
            have_dbl ? F_DOUBLE_CONSTANT : FLOAT_CONSTANT,
            convert_str_double(buffio), strdup(buffio));
    } else {
        omllint_t v = 0;
        string_to_integer(&v, buffio, 10);
        yylval.val = make_int_enode(v);
    }
    return(CONSTANT);
}

static void
string_to_integer(p, cp, radix)
     omllint_t *p;
     char *cp;
     int radix;
{
    char    ch;
    int     x;
    uint32_t v0, v1, v2, v3;

    v0 = v1 = v2 = v3 = 0;      /* clear */
    for( ; (ch = *cp) != 0 ; cp++ ){
        if (isdigit((int)ch))
            x = ch - '0';
        else if ( isupper((int)ch) )
            x = ch - 'A' + 10;
        else
            x = ch - 'a' + 10;
        v0 = v0 * radix + x;
        v1 = v1 * radix + ((v0 >> 16) & 0xFFFF);
        v2 = v2 * radix + ((v1 >> 16) & 0xFFFF);
        v3 = v3 * radix + ((v2 >> 16) & 0xFFFF);
        v0 &= 0xFFFF;
        v1 &= 0xFFFF;
        v2 &= 0xFFFF;
        if ( v3 & 0x80000000 ){
            error("too large integer constant");
            break;
        }
    }

    *p =
        ((omllint_t)v3 << 48) |
        ((omllint_t)v2 << 32) |
        ((omllint_t)v1 << 16) |
        (omllint_t)v0;
#if 0
    fprintf(stderr, "lex: H = %x, L = %x\n", *hp, *p);
#endif
}

static double
convert_str_double(s)
     char *s;
{
    char v[100];
    register char *t;
    int n;

    if((n = strlen(s)) > 90){
          error("too many digits in floating constant");
          n = 90;
      }
    for(t = v ; n-- > 0 ; s++)
      *t++ = (*s=='d' ? 'e' : *s);
    *t = '\0';
    return(atof(v));
}

#define IS_REL_OP(X) \
    (((X) == '<') || ((X) == '>') || ((X) == '/')	\
     || (((X) == '!')))

/*
 * This routine does classify the fortran statements according to the ad-hoc
 * nature.
 */

static int
classify_statement()
{
    register char *p,*save;
    
    while(isspace(*bufptr)) bufptr++;
    save = bufptr;
    if(bufptr[0] == '\0') return(EOS);

    st_class = get_keyword(keywords);

    /* st_name: ....? */
    if ((st_class == UNKNOWN)
	|| (fixed_format_flag
	    &&
	    (isalpha((int) *bufptr)
	     || isdigit((int) *bufptr) || *bufptr == '_'))) {
	char *startp; /* point begin of st_name.  */
	char *endp; /* end pooint for st_name.  */
	char *endcol; /* point the ':' for replace of blanks.  */
	p = bufptr;
	if (st_class == UNKNOWN) {
	    while (isspace(*p)) p++;     /* skip space */
	    if (!isalpha ((int) *p++))
		goto ret_LET;
	    startp = p - 1;
	} else {
	    startp = save; /* keep the key.  */
	}
	while (isalpha((int) *p)
		|| isdigit((int)*p)
	       || (*p == '_'))
	  p++;
	endp = p;
	while (isspace(*p)) p++;     /* skip space */
	if (*p++ == ':') { /* check whether st_name? */
	    if (*p == ':')
		goto ret_LET; /* '::' */
	    endcol = p - 1;
	    while (isspace(*p)) p++;     /* skip space */
	    bufptr = p; /* copy back the pointer for get_keywords() */
	    st_class = get_keyword(keywords);
	    if(st_class == UNKNOWN)
		goto ret_LET;
	    *endp = '\0';
	    st_name = GEN_NODE (IDENT, find_symbol(startp));
	    /* replace 'st_name :' to blanks.  */
	    memset(startp, (int) ' ', endcol - startp);
	}
	else {
	    if (st_class == UNKNOWN)
		goto ret_LET; /* case for non keyword  */
	}
    }

    p = bufptr;
    while(isspace(*p)) p++;     /* skip space */
    if(*p == '('){
        /* check 'keyword(...) = ...' */
        for(p += 1,paren_level = 1; paren_level != 0; p++) {
            if(*p == '\0') {
                error("unmatch paren");
                return(EOS);
            } else if(*p == ')') paren_level--;
            else if(*p == '(') paren_level++;
            else if(*p == QUOTE) {
                /* skip in string */
                while(*++p != QUOTE){
                    if(*p == '\0'){
                        error("syntax error in string");
                        return EOS;
                    }
                }
            }
        }
        while(isspace(*p)) p++; /* skip space */
        if(*p == '=') goto ret_LET;
    }

    p = bufptr;
    while(isspace(*p)) p++;     /* skip space */
    if(*p == '=') goto ret_LET;

    switch(st_class){
    case DO:
        if(fixed_format_flag && exposed_eql){
            if(!exposed_comma) goto ret_LET;
        }
        if (fixed_format_flag) {
          /* when fixed format, it not blank mandatory for DO WHILE. */
          if (strncmp(bufptr, "while", 5) == 0) {
            bufptr = p + 5;
            return DOWHILE;
          }
        }
        break;

    case LOGIF: /* check whether ret_LET or LOGIF?  */
    case WHERE: /* check whether ret_LET or WHERE?  */
        /* Caution:  must save plevel and need_keyword.
           special handling for '' */
        if (fixed_format_flag && exposed_eql) {
            /* LOGIF such as 'if' '('...')' 'yyy' '=' 'zzz'.  */
            /* or 'if' '(' ...'('...')'...')')  'yyy' '=' 'zzz'.  */
            /* fixed_format_flag here, no need skip white space.  */
            int plevel = 0;
            char *save = bufptr;
            if (*bufptr == '(') { /* now point '(' ? */
                plevel++;
                while (*++bufptr) /* skip until ')'  */
                    if (*bufptr == ')') {
                        if (--plevel == 0) /* close for if cond.?  */
                            break;
                    }
                    else if (*bufptr == '(') {  /* another '('?  */
                        plevel++;
                    }
                    else if(*bufptr == QUOTE) {
                        /* skip in quote string */
                        while(*++bufptr != QUOTE){
                            if(*bufptr == '\0'){
                                error("syntax error in string");
                                return EOS;
                            }
                        }
                    }
                /* match the parenthesis?  */
                if ((plevel == 0) && *bufptr == ')') {
                    bufptr++;
                    if (isalpha(*bufptr)) { /* id?  if (....) 'id'=...*/
                        ++bufptr;
                        while (isalpha(*bufptr)
                               || isdigit(*bufptr)
                               || (*bufptr == '_'))
                            bufptr++;

                        /* id, id(), id(...), etc.is here, so more simple.  */
                        bufptr = save;
                        goto ret_LOGIF;
                    } else if (*bufptr == '=') { /* if(...) = ... */
                        bufptr = save;
                        goto ret_LET;
                    }
                }
            }
            bufptr = save;
            goto ret_LET;
      }
      else { /* check arith if, such as 'if' '(' ... ')' 123,....  */
          int plevel = 0;
          char *save = bufptr;
	  while (isspace(*bufptr)) bufptr++;     /* skip space */
          if (*bufptr == '(') { /* now point '(' ? */
            plevel++;
            while (*++bufptr) /* skip until ')'  */
              if (*bufptr == ')') {
                if (--plevel == 0) /* close?  */
                  break;
              }
              else if (*bufptr == '(') {  /* another '('?  */
                plevel++;
              }
	      else if(*bufptr == QUOTE) {
		  /* skip in string */
		  while(*++bufptr != QUOTE){
		      if(*bufptr == '\0'){
			  error("syntax error in string");
			  return EOS;
		      }
		  }
	      }
            if ((plevel == 0) && *bufptr == ')') {
	      /* match the parenthesis?  */
	      bufptr++;
	      /* if (...) then ?  */
	      while (isspace(*bufptr)) bufptr++;     /* skip space */
              if (isalpha(*bufptr)) {
		int save_n = need_keyword;
		int save_p = paren_level;
		need_keyword = 1;
		int t = token();
		if (t == THEN) { /* then key?  */
		  bufptr = save; /* it is IFTHEN statement, not LET.  */
		  return IFTHEN;
		}
		need_keyword = save_n;
                bufptr = save;
		paren_level = save_p;
                break;
              }
              else if (isdigit(*bufptr)) {
                bufptr = save; /* it is ARITHF statement, not LET.  */
                return ARITHIF;
              }
            }
          }
	  bufptr = save; /* resore the pointer.  */
      }
    ret_LOGIF:
      break;

    case ALLOCATABLE:
    case ALLOCATE:
    case CASE:
    case COMMON:
    case CONTAINS:
    case CONTINUE:
    case CYCLE:
    case DATA:
    case DEALLOCATE:
    case DIMENSION:
    case CODIMENSION:
    case ELSE:
    case END:
    case ENTRY:
    case EQUIV:
    case EXIT:
    case EXTERNAL:
    case FUNCTION:
    case GOTO:
    case IMPLICIT:
    case INCLUDE:
    case INTENT:
    case INTERFACE:
    case INTRINSIC:
    case KW_BLOCK:
    case KW_GO:
    case KW_IN:
    case KW_KIND:
    case KW_LEN:
    case KW_OUT:
    case KW_TO:
    case KW_TYPE:
    case KW_USE:
    case MODULE:
    case NAMELIST:
    case NULLIFY:
    case OPTIONAL:
    case PARAMETER:
    case POINTER:
    case PRIVATE:
    case SAVE:
    case SELECT:
    case SEQUENCE:
    case STOP:
    case SUBROUTINE:
    case TARGET:
        if(fixed_format_flag && exposed_eql) {
          goto ret_LET;
        }
        break;

    case PUBLIC:
	may_generic_spec = TRUE;
	break;

    case READ:
        if(*p == '('){  /* read( */
            bufptr = p+1;
            return READ_P;
        }
        break;
    case WRITE:
        if(*p == '('){  /* write( */
            bufptr = p+1;
            return WRITE_P;
        }
        break;
    case REWIND:
        if(*p == '('){  /* rewind( */
            bufptr = p+1;
            return REWIND_P;
        }
        break;
    case ENDFILE:
        if(*p == '('){  /* endfile( */
            bufptr = p+1;
            return ENDFILE_P;
        }
        break;
    case BACKSPACE:
        if(*p == '('){  /* backspace( */
            bufptr = p+1;
            return BACKSPACE_P;
        }
        break;
    }
    return(st_class);

ret_LET:
    st_class = LET; /* if(1) = ??? */
    bufptr = save;
    return LET;
}

/*
  replace pareln, (/) to </> in implicit arg

  why & because:
   in LALAR(1) limitation, can parse following two lines into separate rule.

     implict real(4) (a-h, o-z)
     implicit real (a-h, o-z)

   so we chagne the paren in letter group from (/) to </>(LT/GT).
*/
static
void replace_paren() 
{
    char *lastRpar, *lastLpar;
    int plevel;

nextPair:
    while (isspace(*bufptr)) bufptr++;   /* skip space */
    for (lastLpar = 0, lastRpar = 0, plevel = 0; *bufptr; bufptr++) {
	if (*bufptr == '(') {
	    plevel++;
	    if (plevel == 1)
		lastLpar = bufptr;
	}
	else if (*bufptr == ')') {
	    if (plevel == 1)
		lastRpar = bufptr;
	    plevel--;
	}
	else if (*bufptr == ',') {
	    if (plevel == 0)
		break;
	}
    }
    if ((lastLpar != 0) && (lastRpar != 0)
	&& (*lastLpar == '(') && (*lastRpar == ')')) {
	*lastLpar = '[';
	*lastRpar = ']';
	if (*bufptr == '\0')
	    return;
    }
    else if (*bufptr == '\0')
	return;
    bufptr++;
    goto nextPair;
}

/* check tokn is syntax name or not. */
static int
is_not_keyword()
{
    char *save = bufptr;
    int ret = FALSE;
    bufptr--;
    while (isalnum(*bufptr) || *bufptr == '_')
        bufptr++;  /* skip other character. */
    while (isspace(*bufptr))
        bufptr++;  /* skip white space. */
    if (lexstate == LEX_FIRST_TOKEN &&
        *bufptr == ':' && *(bufptr+1) != ':') {
        /* idxxx ':' .. id is st_name. */
        ret = TRUE;
    }
    else if (exposed_eql && *bufptr == '%') {
        /* id%elment = .. and id has same name as keyword.  */
        ret = TRUE;
    }

    bufptr = save;
    while (isalnum(*bufptr) || *bufptr == '_')
        bufptr++;  /* skip other character. */
    if(bufptr != save) {
        while (isspace(*bufptr))
            bufptr++;  /* skip white space. */
        if (*bufptr == '=') {
            /* idxxx = .. and id has same name as keyword.  */
            ret = TRUE;
        }
    }
    bufptr = save;
    return ret;
}

/* cut keyword from st_buffer */
/* serch for keyword (BAKA search) */
static int
get_keyword(ks)
     struct keyword_token *ks;
{
    register char *p,*q;
    struct keyword_token *kp;
    char *save;

    if(!isalpha((int)*bufptr)) return(UNKNOWN);

    save = bufptr;

    if(fixed_format_flag) {
        for(kp = ks; kp->k_name; kp++) {
            q = kp->k_name;
            if(*q == '_') break;        /* debug hook */
            for(p = bufptr;             /* */; p++,q++){
                if(*q == '\0') {        /* found */
                    bufptr = p;         /* cut off keyword */
                    switch(kp->k_token) {
                    case IMPLICIT: {
                        char *save = bufptr;
                        if (get_keyword(ks) == KW_NONE)
                            return IMPLICIT_NONE;
                        if (is_not_keyword())
                            goto unknown;
                        /* replace the (/) around letter group to </>. */
                        bufptr = save;
                        replace_paren();
                        bufptr = save;
                    }
                        break;
                    case CASE: {
                        char *save = bufptr;
                        if (get_keyword(ks) == KW_DEFAULT)
                            return CASEDEFAULT;
                        else
                            bufptr = save;
                    }
                        break;
                    case DO: {
                        return DO;
                    }
                        break;
                    default:
                        break;
                    }
                    if(is_not_keyword())
                        goto unknown;
                    return(kp->k_token);
                }
                else if(*p != *q) break;
                /* else continue */
            }

        }
    unknown:
        bufptr = save;
        return(UNKNOWN);

    } else {
        int tkn_len;
        char *p;
        int class,cl;
        int ret = UNKNOWN;
        /*  'save' is an original point of buffer.
         * If token is unknown then return this.
         * (read_identifier use it.)
         */
        char *keyword_save = save;
        /*  'keyword_save' will be a point of buffer after read fortran keyword.
         * If token is fortran keyword then return this.
         */

        p = buffio;
        for(tkn_len = 0;
            isalpha((int)*bufptr) || isdigit((int)*bufptr) || 
                *bufptr == '_' || *bufptr == '.'; 
            p++, bufptr++){
            if(tkn_len++ > MAX_NAME_LEN && is_using_module == FALSE) {
                error("name is too long");
            }
            if(tkn_len > 1 && *(p-1) == '.') break;  /* dot_keyword */
            *p = *bufptr;
        }
        if (exposed_eql) {
            /* id%elment = .. and id has same name as keyword.  */
            while (isspace(*bufptr))
                bufptr++;  /* skip white space */
            if (*bufptr == '%') {
                bufptr = save;
                return UNKNOWN;
            }
        }
        *p = 0;         /* termination */
        for(kp = ks; kp->k_name; kp++){
            if(strcmp(kp->k_name,buffio) == 0){
                class = kp->k_token;
                ret = class;
                if(ks == keywords){
                    if((cl = get_keyword_optional_blank(class)) != UNKNOWN) {
                        ret = cl;
                    }
                }
                keyword_save = bufptr;
                break;
            }
        }
        bufptr = keyword_save; /* a point after keyword. */

        if (bufptr > save && lexstate == LEX_FIRST_TOKEN) {
            while (isalnum(*bufptr) || *bufptr == '_')
                bufptr++;
            while (isspace(*bufptr))
                bufptr++;  /* skip white space */
            if (*bufptr == ':' && *(bufptr+1) != ':') {
                /* id ':' .. id is st_name. */
                ret = UNKNOWN;
                bufptr = save;
            } else {
                bufptr = keyword_save;
            }
        }

        return ret;
    }
}


static int
get_keyword_optional_blank(int class)
{
    int cl;
    char *save;

    save = bufptr;
    while(isspace(*bufptr)) bufptr++;   /* skip space */

    switch(class){
    case END:
        if((cl = get_keyword(end_keywords)) != UNKNOWN){
            if(cl == KW_BLOCK){
                if(get_keyword(keywords) == DATA) return ENDBLOCKDATA;
                break;
            }
            return cl;
        }
        break;
    case KW_BLOCK: /* BLOCK DATA*/
        if(get_keyword(keywords) == DATA) return BLOCKDATA;
        break;
    case KW_ENDBLOCK: /* BLOCK DATA*/
        if(get_keyword(keywords) == DATA) return ENDBLOCKDATA;
        break;

    case KW_DBL: { /* DOBULE PRECISION */
		 /* DOBULE COMPLEX */
	   char *save2 = bufptr;
	   if(get_keyword(keywords) == KW_PRECISION) return KW_DOUBLE;
	   bufptr = save2; /* recover and search */
	   if(get_keyword(keywords) == KW_COMPLEX) return KW_DCOMPLEX;
        }
        break;

    case ELSE:  /* ELSE IF */
        if(get_keyword(keywords) == LOGIF)  return ELSEIFTHEN;
        break;
    case KW_GO:
        if(get_keyword(keywords) == KW_TO) return GOTO;
        break;
    case KW_IN:
        if(get_keyword(keywords) == KW_OUT) return KW_INOUT;
        break;
    case KW_SELECT:
        if(get_keyword(keywords) == CASE) return SELECT;
        break;
    case DO: /* DO WHILE *//* blanks mandatory.  */
        if(get_keyword(keywords) == KW_WHILE) return DOWHILE;
        break;
    case IMPLICIT: {/* IMPLICIT NONE */ /* in free format */
	    char *save2 = bufptr;
            if (get_keyword(keywords) == KW_NONE) return IMPLICIT_NONE;
	    bufptr = save2;
	    replace_paren();
        }
	break;
    case CASE: /* case default */
        if(get_keyword(keywords) == KW_DEFAULT) return CASEDEFAULT;
	break;
    case INTERFACE: /* interface assignment or interface operator */ {
           char *save2 = bufptr;
           if(get_keyword(keywords) == ASSIGNMENT) return INTERFACEASSIGNMENT;
	   bufptr = save2;
	   if(get_keyword(keywords) == OPERATOR) return INTERFACEOPERATOR;
        }
	break;
    case MODULE: /* module procedure */
        if (get_keyword(keywords) == PROCEDURE) return MODULEPROCEDURE;
	break;
    default:
        break;
    }

    bufptr = save;      /* recover */
    return UNKNOWN;
}


int checkInsideUse()
{
    return is_using_module;
}


void
setIsOfModule(ID id)
{
    ID_IS_OFMODULE(id) = checkInsideUse();
}

void set_function_disappear()
{
    function_apperable = FALSE;
}

void set_function_appearable()
{
    function_apperable = TRUE;
}

/*
 * for include statement
 *
 * 1. search name.
 * 2. if name is not either of abs. path and current/upper path,
 *    search name in include dir.
 */
void
include_file(char *name, int inside_use)
{
    char buff[MAX_PATH_LEN];
    extern char *includeDirv[];
    extern int includeDirvI;
    int i;
    FILE *fp;
    struct saved_file_state *p;

    if (n_nested_file >= N_NESTED_FILE) {
        error("too nested include file (max = %d)",N_NESTED_FILE);
        exit(EXITCODE_ERR);
    }

    if ((fp = fopen(name, "r")) == NULL) {
        int len = strlen(name);
        if (includeDirvI <= 0 ||    
            name[0] == '/' ||
            (len > 1 && name[0] == '.' && name[1] == '/') ||
            (len > 2 && name[0] == '.' && name[1] == '.' && name[2] == '/')) {
            error("cannot open file '%s'",name);
            exit(EXITCODE_ERR);
        }

        for (i = 0; i < includeDirvI; i++) {
            strcpy(buff, includeDirv[i]);
            strcat(buff, "/");
            strcat(buff, name);
            if ((fp = fopen (buff, "r")) != NULL)
                break;
        }
        if (fp == NULL) {
            error("cannot open file '%s'", name);
            exit(EXITCODE_ERR);
        }
    }

    assert(fp);

    p = &file_state[n_nested_file++];
    p->save_fp = source_file;
    p->save_line = current_line;
    p->save_pre_read = pre_read;
    p->save_lineno = read_lineno;
    p->is_using_module = is_using_module;
    /* save the context for fixed_format_flag,
     * since we force change in USE.
     */
    p->save_fixed_format_flag = fixed_format_flag;
    /* save the context for no count up of line number.  */
    p->save_no_countup = no_countup;
    is_using_module = inside_use;

    if(p->save_buffer == NULL){
        if((p->save_buffer = (char *)malloc(sizeof(char)*LINE_BUF_SIZE)) == NULL){
            fatal("cannot allocate memory");
        }
    }
    bcopy(line_buffer,p->save_buffer,LINE_BUF_SIZE);
    if(p->save_stn_cols == NULL){
        if((p->save_stn_cols = (char *)malloc(sizeof(char)*7)) == NULL) {
            fatal("cannot allocate memory");
        }
    }
    bcopy(stn_cols, p->save_stn_cols, 7);
    /* set new state */
    source_file = fp;
    if (inside_use) {
        /* in include_file invoked by USE, we must free format.  */
        /* sincse .modf is free format.  */
        fixed_format_flag = FALSE;
    } else {
        read_lineno.file_id = get_file_id(name);
        read_lineno.ln_no = 0;
    }

    pre_read = 0;
}

static void restore_file()
{
    struct saved_file_state *p;

    fclose(source_file);
    p = &file_state[--n_nested_file];
    assert(p->save_fp);
    source_file = p->save_fp;
    current_line = p->save_line;
    pre_read = p->save_pre_read;
    read_lineno = p->save_lineno;
    /* anyway restore, it may change in use.  */
    fixed_format_flag = p->save_fixed_format_flag; 
    /* restore the no conputup var. for no need line number count up.  */
    no_countup = p->save_no_countup;
    bcopy(p->save_buffer,line_buffer,LINE_BUF_SIZE);
    bcopy(p->save_stn_cols,stn_cols,7);

    if (is_using_module == TRUE) {
        pop_filter();
    }

    is_using_module = p->is_using_module;
}

/*
 * Files name table
 */
int get_file_id(char *file)
{
    int i;
    for(i = 0; i < n_files; i++){
        if(strcmp(file_names[i],file) == 0) return i;
    }
    if(n_files >= MAX_N_FILES)
      fatal("too many files (max No. of files = %d)",MAX_N_FILES);
    file_names[i = n_files++] = strdup(file);
    return i;
}


/* 
 * line reader
 */
static int 
read_initial_line()
{
    int ret;
    st_name = NULL;

    if(fixed_format_flag) {
        prelast_initial_line_pos = last_offset[1];
        ret = read_fixed_format();
        last_initial_line_pos = last_offset[0];
    } else {
        ret = read_free_format();
        prelast_initial_line_pos = last_initial_line_pos;
        last_initial_line_pos = ftell(source_file);
    } 
    return ret;
}

static int
find_last_ampersand(char *buf,int *len)
{
    int l;
    for(l = *len - 1; l > 0; l--){
        if(isspace(buf[l])) continue;
        if(buf[l] == '&'){
            *len = l;
            return TRUE;
        } else return FALSE;
    }
    return FALSE;
}

static int 
is_PRAGMA_sentinel(char **pp)
{
    int i;
    char *p;

    p = *pp;
    memset(stn_cols, ' ', 6);
    while(isspace(*p)) p++;     /* skip space */
    if(*p == '!'){
        /* check sentinels */
        for(i = 0; i < 6; i++,p++) {
            if(*p == '\0' || isspace(*p)) break;
            if (PRAGMA_flag)
                stn_cols[i] = *p;
            else
                stn_cols[i] = TOLOWER(*p);
        }
        stn_cols[i] = '\0';
        if (strncasecmp(stn_cols,"!$", 2) == 0) {
            *pp = p;
            return TRUE;
        }
    }
    return FALSE;
}

static int 
is_OMP_sentinel(char **pp)
{
    int i;
    char *p;

    p = *pp;
    memset(stn_cols, ' ', 6);
    while(isspace(*p)) p++;     /* skip space */
    if(*p == '!'){
	/* check sentinels */
	for(i = 0; i < 6; i++,p++){
	    if(*p == '\0' || isspace(*p)) break;
	    if (PRAGMA_flag)
		stn_cols[i] = *p;
	    else
		stn_cols[i] = TOLOWER(*p);
	}
	stn_cols[i] = '\0';
	if (strcasecmp(stn_cols,"!$omp") == 0) {
	    *pp = p;
	    return TRUE;
	}
    }
    return FALSE;
}

static int last_char_in_quote_is_quote = FALSE;

static int
read_free_format()
{
    int rv = ST_EOF;
    int st_len;
    char *p, *q, *pp;
    int inH = FALSE;
    int hLen = 0;
    int qChar = '\0';
    int l;
    int inQuote = FALSE;
    char *bufMax = st_buffer + ST_BUF_SIZE - 1;
    char oBuf[65536];

    exposed_comma = 0;
    exposed_eql = 0;
    paren_level = 0;
    may_generic_spec = FALSE;

again:
    rv = readline_free_format();
    if(rv == ST_EOF) return rv;

    /* for first line, check line number and sentinels */
    p = line_buffer;
    while(isspace(*p)) p++;     /* skip space */
    st_no = 0;
    st_OMP_flag = FALSE;
    st_PRAGMA_flag = FALSE;
    if (flag_force_c_comment && /* enabel c-comment?  */
        p - line_buffer < 6) {
        if ((p[0] == 'c') || ((p[0] == 'C'))) {
            char * pp = p;
            while (*pp == 'c' || *pp == 'C') *pp = '!'; /* replace c comment. */
            if(is_PRAGMA_sentinel(&p)) {
                st_PRAGMA_flag = TRUE;
                if (!fixed_format_flag) {
                    /* save head of pragma line.  */
                    set_pragma_str(&stn_cols[2]);
                    append_pragma_str(bufptr); /* append the rest of line.  */
                }
            }
            else goto again;  /* comment line */
        }
    }
    if(isdigit(*p)){
        while (isdigit((int)*p))
            st_no = 10 * st_no + (*p++ - '0');
        if(st_no == 0) error("line number must be non-zero");
        if(!isspace(*p) && *p != '\0')
            warning("separator required after line number");
    } else if(*p == '!'){
        /* check sentinels */
        if (is_OMP_sentinel(&p)) { 
            st_OMP_flag = TRUE;
            set_pragma_str(&stn_cols[2]); /* save head of pragma line.  */
        }
        else if(is_PRAGMA_sentinel(&p)) {
            st_PRAGMA_flag = TRUE;
            set_pragma_str(&stn_cols[2]); /* save head of pragma line.  */
            append_pragma_str(bufptr); /* append the rest of line.  */
        }
        else goto again;  /* comment line */
    }

    /* copy body */
    if (debug_flag) {
        debugOutStatement();
    }
    
    /* first line number is set */
    current_line = new_line_info(read_lineno.file_id,read_lineno.ln_no);

    if (!st_OMP_flag) {
        /* copy to statement buffer */
        q = st_buffer;
        st_len = ScanFortranLine(p, p, q, q, bufMax,
                                 &inQuote, &qChar, &inH, &hLen, &p, &q);
    } else {
        strcpy(st_buffer, p);
        st_len = strlen(st_buffer);
    }
    st_buffer[st_len] = '\0';

    if (st_len >= ST_BUF_SIZE){
        goto Done;
    }
    
    while(find_last_ampersand(st_buffer,&st_len)){
    next_line:
        rv = readline_free_format();
        if(rv == ST_EOF){
            error("unexpected EOF");
            return rv;
        }
        p = line_buffer;

        if(st_OMP_flag){
            if(!is_OMP_sentinel(&p)){
                error("bad OMP sentinel continuation line");
                goto Done;
            }
        } else {
            while(isspace(*p)) p++;
            if(*p == '&') p++; /* double ampersand */
            else {
                if (*p == '!')  /* skip comment line.  */
                    goto next_line;
                else if (*p == '\0') /* skip blank line.  */
                    goto next_line;
                p = line_buffer;  /* reset */
            }
        }

        memcpy(oBuf, st_buffer, st_len);
        pp = oBuf + st_len;

	if (last_char_in_quote_is_quote){
	  *pp = qChar;
	  last_char_in_quote_is_quote = FALSE;
	  l = strlen(p);
	  memcpy(pp+1, p, l); /* oBuf <= line_buffer */
	  *(p + 1 + l) = '\0';
	}
	else {
	  l = strlen(p);
	  memcpy(pp, p, l); /* oBuf <= line_buffer */
	  *(pp + l) = '\0';
	}

        /* oBuf => st_buffer */
        q = st_buffer+st_len;
        l = ScanFortranLine(pp, oBuf, q, st_buffer, bufMax,
                                 &inQuote, &qChar, &inH, &hLen, &p, &q);
        st_len += l;
        if (st_len >= ST_BUF_SIZE){
            goto Done;
        }
    }

    if (last_char_in_quote_is_quote){
      *q++ = QUOTE;
      inQuote = FALSE;
      qChar = '\0';
      last_char_in_quote_is_quote = FALSE;
    }

    /* done */
Done:

    if (endlineno_flag)
      if (current_line->ln_no != read_lineno.ln_no)
	current_line->end_ln_no = read_lineno.ln_no;

    if (st_PRAGMA_flag)
        goto Last;

    if (inQuote == TRUE) {
        error("un-closed quotes");
    }
    if (inH == TRUE) {
        warning("un-terminated Hollerith code.");
        if (st_len < ST_BUF_SIZE) {
            *q++ = QUOTE;
            *q = '\0';
        }
    }

Last:
    memcpy(st_buffer_org, st_buffer, st_len);
    st_buffer_org[st_len] = '\0';

    return ST_ONE;
}

/* check the module compile? and if reach the end of offset?  */
static int anotherEOF() {
  extern int mcLn_no;
  extern long mcEnd;

  if (n_nested_file == 0 && mcLn_no != -1)
    {
      long l = ftell(source_file);
      if (l > mcEnd)
        return TRUE;
    }
  return FALSE;

}

static int
readline_free_format()
{
    int c,i;
    char *bp,*p;
    int inQuote;
    int inComment;
    int len = strlen(line_buffer);

    if(line_count > 1  && find_last_ampersand(line_buffer, &len)) {
        inQuote = prevline_is_inQuote;
        inComment = prevline_is_inComment;
    } else {
        inQuote = 0;
        inComment = FALSE;
    }

    /* read initial line */
next_line:
    if (!no_countup)
    /* count up line# counter in cpp's one.  */
	read_lineno.ln_no++;
    last_ln_nos[1] = last_ln_nos[0];

next_line0:
    last_ln_nos[0] = read_lineno.ln_no;
    last_ln_no = last_ln_nos[0];

    /* count up line# counter in whole read.  */
    if (!no_countup)
        line_count++;
    no_countup = FALSE;
    c = getc(source_file);
    if(c == EOF) return ST_EOF;
    if (anotherEOF()) return ST_EOF;
    ungetc(c,source_file);
/* handling the case string has ';'. */
/* should handlig for holirith here?  */
#if 0
    for(bp = line_buffer, inQuote = 0, inComment = FALSE;
#endif
    for(bp = line_buffer;
        bp < &line_buffer[LINE_BUF_SIZE]; ) {
        c = getc(source_file);
        if(c == '\n') goto done;
        if (!inQuote && !inComment && (c == ';')) {
            no_countup = TRUE;
            goto done;
        }
        //if(c == EOF) goto unexpected_EOF;
	if (c == EOF) goto done;
        if (anotherEOF()) goto unexpected_EOF;
        if (!inComment && (c == '\'')) {
            if (inQuote == c) /* terminate?  */
                inQuote = 0;
            else if (!inQuote)
                inQuote = c;
        }
        else if (!inComment && (c == '"')) {
            if (inQuote == c) /* terminate? */
                inQuote = 0;
            else if (!inQuote)
                inQuote = c;
        }
        else if (!inQuote && !inComment && (c == '!'))
            inComment = TRUE;
        *bp++ = c;
    }
    error("too long line, skipped");
    while((c = getc(source_file)) != '\n'){
      //if(c == EOF) goto unexpected_EOF;
      if (c == EOF) break;
      if (anotherEOF()) goto unexpected_EOF;
    }
    goto next_line;

unexpected_EOF:
    error("unexpected EOF");
    return ST_EOF;

done:
    *bp = '\0';
    prevline_is_inQuote = inQuote;
    prevline_is_inComment = inComment;
    if(line_buffer[0] == '#'){
        prevline_is_inQuote = 0;
        prevline_is_inComment = FALSE;
        bp = line_buffer;
        bp++;
        while(*bp == ' ' || *bp == '\t' || *bp == '\b')
            bp++;        /* skip space */
        if(isdigit((int)(*bp))){
            i = 0; /* line # reset default '0' */
            while(isdigit((int)(*bp)))
                i = i * 10 + *bp++ - '0';
            read_lineno.ln_no = i;
            while(*bp == ' ') bp++;    /* skip space, again */
            if (*bp == '"'){           /* parse file name */
                bp++;
                p = buffio;            /* rewrite buffer file name */
                while(*bp != '"' && *bp != 0) *p++ = *bp++;
                *p = '\0';
                read_lineno.file_id = get_file_id(buffio);
                /* if first line is #line, then set it
                   as original source file name */
                if(line_count == 1)
                    source_file_name = strdup(buffio);
            }
        } else warning("bad # line");
        goto next_line0;
    }
    if(bp - line_buffer > 132)
        warning("line contains more than 132 characters");
    return(ST_INIT);
}

/* for fixed format */
static int
read_fixed_format()
{
    int rv = ST_EOF;
    int st_len, i;
    char *p, *q;
    int inH = FALSE;
    int hLen = 0;
    int qChar = '\0';
    char *bufMax = st_buffer + ST_BUF_SIZE - 1;
    char oBuf[65536];
    int newLen;
    int lnLen;
    int inQuote = FALSE;
    int current_st_PRAGMA_flag = 0;

    exposed_comma = 0;
    exposed_eql = 0;
    paren_level = 0;
    may_generic_spec = FALSE;

top:
    if (!pre_read) {
        pre_read = 0;
        if ((rv = readline_fixed_format()) == ST_EOF) {
            return ST_EOF;
        }
        if (rv == ST_CONT) {
            error("missing initial line");
            goto top;
        }
    }

    st_no = 0;
    st_len = 0;
    st_PRAGMA_flag = FALSE;
    st_OMP_flag = FALSE;

    if (stn_cols[0] == 'c'){

      if (strncasecmp(&stn_cols[1],"$omp",4) == 0){
        st_OMP_flag = TRUE;
        set_pragma_str(&stn_cols[2]);
        append_pragma_str (line_buffer);
        goto copy_body;
      }
      else if (stn_cols[1] == '$' && stn_cols[2] != '$'){
        st_PRAGMA_flag = TRUE;
        set_pragma_str(&stn_cols[2]);
        append_pragma_str (line_buffer);
        if (debug_flag)
	  printf("pragmaString(%s)\n", pragmaString);
        goto copy_body;
      }
      else { /* comment line */
	goto top;
      }
    }

/*     if (strncasecmp(&stn_cols[1],"$omp",4) == 0) { */
/*         st_OMP_flag = TRUE; */
/* 	goto copy_body; */
/*     } */

    /* get line number */
    for (p = stn_cols, i = 0; *p != '\0' && i < 5; p++, i++) {
        if (!isspace((int)*p)) {
            if (isdigit((int)*p)) {
                st_no = 10 * st_no + (*p - '0');
            } else {
                error("no digit in statement nubmer field");
                st_no = 0;
                break;
            }
        }
    }

copy_body:
    if (debug_flag) {
        debugOutStatement();
    }
    
    /* first line number is set */
    current_line = new_line_info(read_lineno.file_id,read_lineno.ln_no);

    /* copy to statement buffer */
    p = line_buffer;
    q = st_buffer;
    newLen = st_len = ScanFortranLine(p, p, q, q, bufMax,
                                      &inQuote, &qChar, &inH, &hLen, &p, &q);
    st_buffer[newLen] = '\0';
    if (st_len >= ST_BUF_SIZE) {
        goto Done;
    }
    memcpy(oBuf, st_buffer, newLen);
    current_st_PRAGMA_flag = st_PRAGMA_flag;
    while ((rv = readline_fixed_format()) == ST_CONT) {

        if (strncasecmp(&stn_cols[1],"$omp",4) == 0) {
            if (st_OMP_flag) {
                append_pragma_str (" ");
                append_pragma_str (line_buffer);
                goto copy_body_cont;
            }
            error("OMP sentinels missing initial line, ignored");
            break;
        } else if (st_OMP_flag) {
            error("continue line follows OMP sentinels, ignored");
            break;
        } else if (st_PRAGMA_flag) {
            set_pragma_str (&stn_cols[2]);
            append_pragma_str (line_buffer);
            goto copy_body_cont;
        }

        for (p = stn_cols, i = 0; *p != '\0' && i < 5; p++, i++) {
            if (!isspace((int)*p)) {
                warning("statement label in continuation line is ignored");
                break;
            }
        }
        if (debug_flag) {
            debugOutStatement();
        }

    copy_body_cont:

        /* copy to statement buffer */
	p = oBuf + st_len;

	if (last_char_in_quote_is_quote){
	  *p = qChar;
	  last_char_in_quote_is_quote = FALSE;
	  lnLen = strlen(line_buffer);
	  memcpy(p+1, line_buffer, lnLen); /* oBuf <= line_buffer */
	  *(p + 1 + lnLen) = '\0';
	}
	else {
	  lnLen = strlen(line_buffer);
	  memcpy(p, line_buffer, lnLen); /* oBuf <= line_buffer */
	  *(p + lnLen) = '\0';
	}

        /* oBuf => st_buffer */
        newLen = ScanFortranLine(p, oBuf, q, st_buffer, bufMax, 
                                 &inQuote, &qChar, &inH, &hLen, &p, &q);
        st_len += newLen;
        if (st_len >= ST_BUF_SIZE) {
            goto Done;
        }
        /* oBuf <= st_buffer, copy back */
        memcpy(oBuf, st_buffer, st_len);

    if (endlineno_flag)
      if (current_line->ln_no != read_lineno.ln_no)
	current_line->end_ln_no = read_lineno.ln_no;

    }

    if (last_char_in_quote_is_quote){
      *q++ = QUOTE;
      inQuote = FALSE;
      qChar = '\0';
      last_char_in_quote_is_quote = FALSE;
    }

    *q = '\0';                  /* termination */

Done:

    st_PRAGMA_flag = current_st_PRAGMA_flag;

    if (st_PRAGMA_flag)
        goto Last;

    if (inQuote == TRUE) {
        error("un-closed quotes");
    }

    if (inH == TRUE) {
        warning("un-terminated Hollerith code.");
        if (st_len < ST_BUF_SIZE) {
            *q++ = QUOTE;
            *q = '\0';
        }
    }

Last:
    memcpy(st_buffer_org, st_buffer, st_len);
    st_buffer_org[st_len] = '\0';

#if 0
    fprintf(stderr, "debug: read_initial got '%s'\n", st_buffer);
#endif

    pre_read = (rv == ST_INIT) ? 1 : 0;
    return ST_ONE;
}


/* fixed format: read one line from input, check comment line */
static int
readline_fixed_format()
{
    register int c,i;
    char *bp,*p;
    int maxChars = 0;
    int check_cont; /* need to check the contination line.  */
    int inQuote = 0;
    int inComment = FALSE;

    /* line # counter for each file in cpp.  */
next_line:
    if (!no_countup)
	read_lineno.ln_no++;

/* keep last line offset and line number.  */
    last_offset[1] = last_offset[0];
    last_offset[0] = ftell(source_file);
    last_ln_nos[1] = last_ln_nos[0];
    last_ln_no = last_ln_nos[1];
    last_ln_nos[0] = read_lineno.ln_no;

    if (debug_flag) {
	printf ("readline_fixed_format(): %d/%ld\n",
            last_ln_nos[0], last_offset[0]);
    }
    
    /* total # of line for read.  */
next_line0:
    if (!no_countup)
        line_count++;
    no_countup = FALSE;
/* next_lineSep: next line after ';'.  */
    check_cont = TRUE; /* need the cont. line ? */
    memset(line_buffer, 0, LINE_BUF_SIZE);
    memset(stn_cols, ' ', 6);
    stn_cols[6] = '\0';
    c = getc(source_file);
    /* reach the end in MC.  */
    if (anotherEOF()) return ST_EOF;
    switch(c){
    case '#': /* skip cpp line */
        for(bp = line_buffer; bp < &line_buffer[LINE_BUF_SIZE]; ){
            if((c = getc(source_file)) == '\n') break;
            *bp++ = c;
        }
        *bp = '\0';
        bp = line_buffer;
        while(*bp == ' ' || *bp == '\t' || *bp == '\b')
            bp++;        /* skip space */
        if(isdigit((int)(*bp))){
            i = 0; /* line # reset default '0' */
            while(isdigit((int)(*bp)))
                i = i * 10 + *bp++ - '0';
            read_lineno.ln_no = i;
            while(*bp == ' ') bp++;    /* skip space, again */
            if (*bp == '"'){   /* parse file name */
                bp++;
                p = buffio;       /* rewrite buffer file name */
                while(*bp != '"' && *bp != 0) *p++ = *bp++;
                *p = '\0';
                read_lineno.file_id = get_file_id(buffio);
                /* if first line is #line, then set it
                   as original source file name */
                if(line_count == 1)
                    source_file_name = strdup(buffio);
            }
        } else warning("bad # line");
        goto next_line0;
        
    case 'c':
    case 'C':
    case '*':
    case '!':
        if (PRAGMA_flag) {
            c = getc(source_file);
            if (c == '$') {
                ungetc (c, source_file);
                ungetc ('c',source_file);
                goto read_num_column;
            }
            ungetc (c, source_file);
            ungetc ('c',source_file);
        }

    read_comment:
    while((c = getc(source_file)) != '\n') {
#if 0
        if (c == '$') {
            ungetc (c, source_file);
            ungetc ('c',source_file);
            goto read_num_column;
        }
#endif
        if(c == EOF) return(ST_EOF);
        if (anotherEOF()) return(ST_EOF);
    }
        goto next_line;
    case EOF:
        return(ST_EOF);
    case '\n':
        /* blank line */
      //goto next_line;
      return ST_INIT;
    default:
        /* read line number column */
        ungetc(c,source_file);
    read_num_column:
        for(i = 0; i < 6 ;i++) {
            c = getc(source_file);
            if (c == EOF) {
	      //warning("unexpected eof");
	      //return(ST_EOF);
	      while (i < 6) stn_cols[i++] = ' ';
            } else if (anotherEOF()) {
                warning("unexpected eof");
                return(ST_EOF);
            } else if (c == '\n') {
                while (i < 6) stn_cols[i++] = ' ';
		if (stn_cols[0] == 'c') /* only for comment.  */
		  /* if not, line count is over the current.  */
		  ungetc(c,source_file);
                break;
            } else if (c == '\t') {

	      while (i < 5) stn_cols[i++] = ' ';

	      c = getc(source_file);

	      if (c >= '1' && c <= '9'){
		/* TAB + digit indicates a continuation line */
		stn_cols[i] = '1';
	      }
	      else {
                /* skips to column 7 */
		stn_cols[i] = ' ';
		ungetc(c, source_file);
	      }

                /* TAB in col 1-6 skips to column 7 */
                //while (i < 6) stn_cols[i++] = ' ';

                break;
            } else {
	      if (PRAGMA_flag)
		stn_cols[i] = c;
	      else
		stn_cols[i] = isupper(c) ? tolower(c): c;
            }
        }
    if (stn_cols[0] == 'c') {
        if (strncasecmp(&stn_cols[1], "$omp", 4) == 0) {
        /*  OpenMP sentinel no doubt */
        goto KeepOnGoin;
        }
        if (stn_cols[1] == '$') {
        /* Check OpenMP fixed source form conditional compilation. */
        int numSpace = 0;
        int numDigit = 0;

        st_PRAGMA_flag = TRUE;
        if (OMP_flag) {
            check_cont = TRUE;
            /* erase leading "c$" for conditional compilation.  */
            memset(stn_cols, ' ', 2);
        } else
            check_cont = FALSE;

        /*
         * If there are any non-numerical character within
         * column three to five, this line is just a comment.
         */
        for (i = 2; i < 5; i++) {
            if (stn_cols[i] == ' ') {
            numSpace++;
            } else if (isdigit((int)stn_cols[i])) {
            numDigit++;
            }
        }

        if (!OMP_flag)
            goto KeepOnGoin;
        if (numSpace == 3) {
            /* OpenMP conditional compile */
            goto KeepOnGoin;
        } else if ((numSpace + numDigit) == 3) {
            if (IS_CONT_LINE(stn_cols)) {
            /*
             * Invalid continuation line. treat as a
             * comment.
             */
            warning("statement label in continuation line in OpenMP conditional compilation. ignored as a comment.");
            goto read_comment;
            }
            goto KeepOnGoin;
        }
/* no need to warning for another key insted of !$omp.  */
#if 0
		/*
		 * This line seemed to be an OpenMP sentinel/conditional
		 * compilation, but it is not :)
		 */
		warning("unknown sentinel/directive 'c$%s'",
			&stn_cols[2]);
#endif /* 0 */
        }
        goto read_comment;
    }
    }

KeepOnGoin:
    stn_cols[6] = '\0';         /* terminate */
    /* 
     * read body of line
     */
    bp = line_buffer;
    inQuote = 0;
    maxChars = fixed_line_len - 6;
    if (c == '\n') {
        line_buffer[0] = 0;
    } else {
        char scanBuf[4096];
        int scanLen = 0;
        int getNL = FALSE;

        /* handle ';' */
        char c;
        int i;
        for (c = getc(source_file), i = 0;
             i < sizeof(scanBuf);
             c = getc(source_file), i++) {
            if (c == EOF)
                return ST_EOF;
            if (c == '\'' && inComment == FALSE) {
                if(c == inQuote)
                    inQuote = 0;
                else if (!inQuote)
                    inQuote = c;
            } else if (c == '"' && inComment == FALSE) {
                if(c == inQuote)
                    inQuote = 0;
                else if (!inQuote)
                    inQuote = c;
            }
            if (c == '!' && inQuote == 0) {
                inComment = TRUE;
            }
            else if (c == ';' && inQuote == 0 && inComment == FALSE) {
                if (!OMP_flag && (strncmp(stn_cols, "c$ ", 3) == 0))
                    ; /* a line like 'c$ ... ;' with -no-omp is not
                         enable multi line as ';'.  */
                else {
                    /* ';' encorce new line, no count up for line
                       number, and ungetc for \t since next line has
                       neither line number nor continuation. */
                    no_countup = TRUE;
                    ungetc((int) '\t', source_file);
                    goto Newline;
                }
            }
            else if (c == '\n') {
            Newline:
                getNL = TRUE;
                inComment = FALSE;
                scanBuf[i] = '\0';
                scanLen = i;
                if (i > maxChars) {
                    goto Crip;
                }
                break;
            }
            scanBuf[i] = c;
        }
        if (i == sizeof(scanBuf)) {
        Crip:
            scanBuf[maxChars] = '\0';
            scanLen = maxChars;
        }

        memcpy(line_buffer, scanBuf, scanLen);
        line_buffer[scanLen] = '\0';
        if (getNL != TRUE) {
            while((c = getc(source_file)) != '\n') {
                if (c == EOF) {
		  //warning("unexpected EOF");
		  //return(ST_EOF);
		  break;
                }
                if (anotherEOF()) {
                    warning("unexpected EOF");
                    return(ST_EOF);
                }
            }
        }
        bp = line_buffer + scanLen;
    }
    
    /* skip null line */
    for (bp = line_buffer; *bp != 0; bp++) {
        if (!isspace((int)*bp)) break;
    }

    if (*bp == 0) {
        /* brank line */
        for (p = stn_cols, i = 0; *p != '\0' && i < 5;  p++,i++ ) {
            if (*p != ' ') {
                warning("statement number in brank line is ignored");
                break;
            }
        }
        goto next_line;
    }
    
    if (check_cont && IS_CONT_LINE(stn_cols))
        return(ST_CONT);
    else
        return(ST_INIT);
}

static int
power10(y)
     int y;
{
    if (y == 0) {
        return 1;
    } else if (y < 0) {
        return 0;
    } else {
        int ret = 1;
        int i;
        for (i = 1; i <= y; i++) {
            ret *= 10;
        }
        return ret;
    }
}

typedef struct {
    char *key;
    int len;
} unHKey;
static unHKey unHToken[] = {
    {"integer", 7},
    {"real", 4},
    {"double", 6},
    {"logical", 7},
    {"character", 9},
    {NULL, 0}
};

static int
getHollerithLength(head, cur, inQuote)
     char *head;        /* start pointer of the room. */
     char *cur;         /* current pointer that is pointing 'H'|'h' */
     int inQuote;       /* if TRUE, 'H'|'h' is in quote. */
{
    int sLen = 0;
    int nTen = 0;

    cur--;
    if (inQuote == TRUE) {
        /*
         * in quote, only successive [0-9]+[Hh] is a valid hollerith,
         * and always we can treat this as a hollerith.
         */
        while (cur >= head) {
            if (isdigit((int)*cur)) {
                sLen += power10(nTen) * ((int)*cur - '0');
                cur--;
                nTen++;
            } else {
                break;
            }
        }
        return sLen;
    }

    /*
     * get length.
     */
    while (cur >= head) {
        if (isspace((int)*cur)) {
            cur--;
        } else if (isdigit((int)*cur)) {
            sLen += power10(nTen) * ((int)*cur - '0');
            cur--;
            nTen++;
        } else {
            break;
        }
    }
    if (sLen <= 0) {
        return 0;
    }

    /*
     * check backword.
     */
    while (cur >= head) {
        if (isspace((int)*cur)) {
            cur--;
        } else {
            break;
        }
    }

    if (cur <= head &&
        !(isalpha((int)*cur))) {
        /*
         * buffer like  ^[ \t]*[0-9]+[Hh].*$
         *
         * Buffer start with a hollerith. may be syntax error, but
         * definitely a hollerith.
         */
        return sLen;
    }

    if (isalpha((int)*cur)) {
        /*
         * buffer like ^.*[A-Za-z]+[ \t]*[0-9]+[Hh].*$
         * must not be a hollerith.
         */
        return 0;
    } else if (*cur != '*') {
        /*
         * buffer like ^.*^[*]+[ \t]*[0-9]+[Hh].*$
         * must be a hollerith.
         */
        return sLen;
    } else {
        /*
         * buffer like ^.*[*]+[ \t]*[0-9]+[Hh].*$
         * first '.*' might be a type specifier or an identifier.
         */
        int i;
        char rbuf[65536];
        char *rbp = rbuf;
        char buf[65536];
        char *bp = buf;
        int found = 0;
        char *kStart;

        cur++;
        while (cur >= head) {
            if (isspace((int)*cur)) {
                cur--;
            } else if (isalpha((int)*cur)) {
                *rbp++ = *cur--;
            } else {
                break;
            }
        }
        *rbp = '\0';
        rbp--;
        
        while (rbp >= rbuf) {
            *bp++ = *rbp--;
        }
        *bp = '\0';

        for (i = 0; unHToken[i].key != NULL; i++) {
            kStart = bp - unHToken[i].len;
            if (kStart < buf) {
                continue;
            } else {
                if (strncasecmp(kStart, unHToken[i].key,
                                unHToken[i].len) == 0) {
                    found++;
                    break;
                }
            }
        }
        if (found > 0) {
            return sLen;
        } else {
            return 0;
        }
    }
}


static int
getEscapeValue(cur, valPtr, newPtr)
     char *cur;         /* current scan point. */
     int *valPtr;       /* return value pointer. */
     char **newPtr;     /* next scan point return. */
{
    int val = 0;
    if (*cur != '\\') {
        if (valPtr != NULL) {
            *valPtr = (int)(*cur++);
        }
        if (newPtr != NULL) {
            *newPtr = cur;
        }
        return TRUE;
    }

    cur++;

    switch ((int)*cur) {
        case '\\': {
            val = '\\';
            cur++;
            break;
        }
        case 't': {
            val = '\t';
            cur++;
            break;
        }
        case 'b': {
            val = '\b';
            cur++;
            break;
        }
        case 'f': {
            val = '\f';
            cur++;
            break;
        }
        case 'n': {
            val = '\n';
            cur++;
            break;
        }
        case 'r': {
            val = '\r';
            cur++;
            break;
        }
        case '0': {
            val = '\0';
            cur++;
            break;
        }
        default: {
            val = '\\';
            break;
        }
    }
    
    if (newPtr != NULL) {
        *newPtr = cur;
    }
    if (valPtr != NULL) {
        *valPtr = val;
    }
    
    return TRUE;
}


static int
unHollerith(cur, head, dst, dstHead, dstMax, inQuotePtr, quoteChar,
            inHollerithPtr, hollerithLenPtr, newCurPtr, newDstPtr)
     char *cur;
     char *head;
     char *dst;
     char *dstHead;
     char *dstMax;
     int *inQuotePtr;
     int quoteChar;
     int *inHollerithPtr;
     int *hollerithLenPtr;
     char **newCurPtr;
     char **newDstPtr;
{
    int hLen = getHollerithLength(head, cur, *inQuotePtr);
    int nGet = 0;
    int rQC = '\0';

#if 0
    fprintf(stderr, "debug: hLen = %d\n", hLen);
#endif
    if (hLen == 0) {
        /*
         * not a hollerith.
         */
        if (*inQuotePtr == TRUE) {
            *dst++ = *cur;
        } else {
            *dst++ = TOLOWER(*cur);
        }
        cur++;
        nGet = 1;
        goto Done;
    }

#if 0
    fprintf(stderr, "debug: dst = 0x%08x, dHead = 0x%08x\n", dst, dstHead);
#endif
    while (dst >= dstHead) {
        dst--;
        if (isdigit((int)*dst)) {
            continue;
        } else {
            break;
        }
    }
    dst++;
#if 0
    fprintf(stderr, "debug: dst = '%s'\n", dst);
#endif
    if (quoteChar == '\'') {
        rQC = '"';
    } else {
        rQC = '\'';
    }

    cur++;
    if (*inQuotePtr == FALSE) {
        *dst++ = QUOTE;
    } else {
        *dst++ = rQC;
    }
    *inHollerithPtr = TRUE;
    while (*cur != '\0' &&
           nGet < hLen &&
           dst <= dstMax) {
        if (*cur == quoteChar) {
            *dst++ = quoteChar;
            nGet++;
            if (*inQuotePtr == TRUE) {
                if (*(cur + 1) == quoteChar) {
                    cur++;
                } else {
                    *inQuotePtr = FALSE;
                    dst--;
                    cur++;
                    nGet = hLen;
                    goto Done;
                }
            }
            cur++;
            continue;
        } else if (*cur == '\\') {
            int val;
            (void)getEscapeValue(cur, &val, &cur);
            *dst++ = val;
            nGet++;
            continue;
        } else {
            *dst++ = *cur++;
            nGet++;
        }
    }

    Done:
    *hollerithLenPtr = hLen - nGet;
    if (*hollerithLenPtr < 0) {
        *hollerithLenPtr = 0;
    }
    if (*hollerithLenPtr == 0) {
        *inHollerithPtr = FALSE;
        if (hLen > 0 && dst <= dstMax) {
            if (*inQuotePtr == FALSE) {
                *dst++ = QUOTE;
            } else {
                *dst++ = rQC;
            }
        }
    }

    *newCurPtr = cur;
    *newDstPtr = dst;
    return TRUE;
}

static int
checkInQuote(cur, dst, inQuotePtr, quoteCharPtr, newCurPtr, newDstPtr)
     char *cur;
     char *dst;
     int *inQuotePtr;
     int *quoteCharPtr;
     char **newCurPtr;
     char **newDstPtr;
{
    if (*inQuotePtr == FALSE) {
        *inQuotePtr = TRUE;
        *quoteCharPtr = *cur;
        cur++;
        *dst++ = QUOTE;
    } else {
        if (*cur == *quoteCharPtr) {
            cur++;
	    if ((fixed_format_flag && *cur == '\0') ||
		(!fixed_format_flag && *cur == '&' && *(cur+1) == '\0')){
	      last_char_in_quote_is_quote = TRUE;
	    }
            else if (*cur != *quoteCharPtr) {
                *dst++ = QUOTE;
                *inQuotePtr = FALSE;
                *quoteCharPtr = '\0';
            } else {
                *dst++ = *quoteCharPtr;
                cur++;
            }
        } else {
            *dst++ = *cur++;
        }
    }
    *newCurPtr = cur;
    *newDstPtr = dst;
    return TRUE;
}


int
ScanFortranLine(src, srcHead, dst, dstHead, dstMax, inQuotePtr, quoteCharPtr,
                inHollerithPtr, hollerithLenPtr, newCurPtr, newDstPtr)
     char *src;
     char *srcHead;
     char *dst;
     char *dstHead;
     char *dstMax;
     int *inQuotePtr;
     int *quoteCharPtr;
     int *inHollerithPtr;
     int *hollerithLenPtr;
     char **newCurPtr;
     char **newDstPtr;
{
    char *cpDst = dst;

    while (*src != '\0' && dst <= dstMax) {
        if (isspace((int)*src)) {
            if (fixed_format_flag && 
                *inQuotePtr == FALSE && *inHollerithPtr == FALSE ){
                src++;
            } else {
                goto copyOne;
            }
        } else if (*src == '!') {
            if (*inQuotePtr == FALSE && *inHollerithPtr == FALSE) {
                break; /* comment */
            } else {
                goto copyOne;
            }
        } else if (*src == '\'' || *src == '"') {
            if (*inHollerithPtr == FALSE) {
                checkInQuote(src, dst, inQuotePtr, quoteCharPtr, &src, &dst);
            } else {
                goto copyOne;
            }
        } else if (*src == 'h' || *src == 'H') {
            if (*inHollerithPtr == FALSE && *inQuotePtr == FALSE) {
                unHollerith(src, srcHead, dst, dstHead, dstMax, 
                            inQuotePtr, *quoteCharPtr,
                            inHollerithPtr, hollerithLenPtr, &src, &dst);
            } else {
                goto copyOne;
            }
        } else if (*src == '\\') {
            if (*inQuotePtr == TRUE || *inHollerithPtr == TRUE) {
                int val;
                getEscapeValue(src, &val, &src);
                *dst++ = val;
            } else {
                goto copyOne;
            }
        } else {
            if (*inQuotePtr != TRUE && *inHollerithPtr != TRUE) {
                if (*src == '(') {
                    ++paren_level;
                } else if (*src == ')') {
                    --paren_level;
                } else if (paren_level == 0) {
                    if (*src == '=') {
			if (IS_REL_OP(*(src - 1)))
			    ; /* like <= .. ? */
			else if ((*(src + 1) == '=') /* || (*(src + 1) == '>') */)
			    ; /* else == ? */
			else
			    exposed_eql++; /* = (or maybe =>) */
                    } else if (*src == ',') {
                        exposed_comma++;
                    }
                }
            }

        copyOne:
            if (*inQuotePtr == TRUE || *inHollerithPtr == TRUE) {
                *dst++ = *src;
            } else {
                *dst++ = TOLOWER(*src);
            }
            src++;
            if (*inHollerithPtr == TRUE) {
                if (*hollerithLenPtr > 0) {
                    (*hollerithLenPtr)--;
                }
                if (*hollerithLenPtr <= 0) {
                    *hollerithLenPtr = 0;
                    *inHollerithPtr = FALSE;
                    if (*inQuotePtr == FALSE) {
                        *dst++ = QUOTE;
                    } else {
                        if (*quoteCharPtr == '\'') {
                            *dst++ = '"';
                        } else {
                            *dst++ = '\'';
                        }
                    }
                }
            }
        }
    }
    *dst = '\0';
    *newCurPtr = src;
    *newDstPtr = dst;
    return dst - cpDst;
}

/* TOKEN DATA */
struct keyword_token dot_keywords[] =
{
    {"and.", AND}, 
    {"or.", OR}, 
    {"not.", NOT}, 
    {"true.", TRUE_CONSTANT},
    {"false.", FALSE_CONSTANT}, 
    {"eq.", EQ}, 
    {"ne.", NE}, 
    {"lt.", LT}, 
    {"le.", LE}, 
    {"gt.", GT}, 
    {"ge.", GE}, 
    {"neqv.", NEQV}, 
    {"eqv.", EQV}, 
    {NULL, 0}
};

/* caution!: longger word should be first than short one.  */
struct keyword_token keywords[ ] = 
{
    { "assignment",     ASSIGNMENT  },
    { "assign",         ASSIGN  },
    { "allocatable",    ALLOCATABLE },
    { "allocate",       ALLOCATE },
    { "backspace",      BACKSPACE },
    { "blockdata",      BLOCKDATA },
    { "block",          KW_BLOCK},      /* optional */
    { "call",           CALL },
    { "character",      KW_CHARACTER, },
    { "close",          CLOSE, },
    { "common",         COMMON },
    { "complex",        KW_COMPLEX },
    { "continue",       CONTINUE  },
    { "contains",       CONTAINS },
    { "cycle",          CYCLE},
    { "case",           CASE},
    { "data",           DATA },
    { "dimension",      DIMENSION  },
    { "codimension",    CODIMENSION  },
    { "doubleprecision",  KW_DOUBLE  },
    { "doublecomplex",  KW_DCOMPLEX },  
    { "double",         KW_DBL },     /* optional */
    { "do",             DO },
    { "default",        KW_DEFAULT},
    { "deallocate",     DEALLOCATE},
    { "while",          KW_WHILE},
    /* { "dowhile",     DOWHILE }, *//* blanks mandatory */
    { "elsewhere",      ELSEWHERE },
    { "elseif",         ELSEIFTHEN },
    { "else",           ELSE },
    { "exit",           EXIT },
    { "enddo",          ENDDO },
    { "endfile",        ENDFILE  },
    { "endif",          ENDIF },
    { "endblock",       KW_ENDBLOCK },
    { "endforall",      ENDFORALL },
    { "endfunction",    ENDFUNCTION },
    { "endinterface",   ENDINTERFACE },
    { "endmodule",      ENDMODULE },
    { "endprogram",     ENDPROGRAM },
    { "endselect",      ENDSELECT },
    { "endsubroutine",  ENDSUBROUTINE },
    { "endtype",        ENDTYPE },
    { "endwhere",       ENDWHERE },
    { "end",            END  },
    { "entry",          ENTRY },
    { "equivalence",    EQUIV  },
    { "external",       EXTERNAL  },
    { "elemental",      ELEMENTAL },
    { "format",         FORMAT  },
    { "function",       FUNCTION  },
    { "forall",         FORALL },
    { "goto",           GOTO  },
    { "go",             KW_GO  },
    { "if",             LOGIF },
    { "implicit",       IMPLICIT },
    { "include",        INCLUDE },
    { "inquire",        INQUIRE },
    { "intrinsic",      INTRINSIC },
    { "integer",        KW_INTEGER  },
    { "interface",      INTERFACE },
    { "intent",         INTENT},
    { "inout",          KW_INOUT},
    { "in",             KW_IN},
    { "logical",        KW_LOGICAL  },
    { "len",            KW_LEN},
    { "kind",           KW_KIND},
    { "module",         MODULE},
    { "namelist",       NAMELIST },
    { "none",           KW_NONE},
    { "nullify",        NULLIFY},
    { "open",           OPEN },
    { "operator",       OPERATOR },
    { "out",            KW_OUT},
    { "optional",       OPTIONAL},
    { "only",           KW_ONLY},
    { "parameter",      PARAMETER },
    { "pause",          PAUSE  },
    { "pointer",        POINTER },
    { "precision",      KW_PRECISION},
    { "print",          PRINT  },
    { "procedure",      PROCEDURE },
    { "program",        PROGRAM },
    { "private",        PRIVATE},
    { "pure",           PURE},
    { "public",         PUBLIC},
    { "result",         RESULT},
    { "recursive",      RECURSIVE},
    /*    { "punch",    PUNCH }, */
    { "read",           READ },
    { "real",           KW_REAL },
    { "return",         RETURN  },
    { "rewind",         REWIND  },
    { "save",           SAVE },
    { "selectcase",     SELECT },
    { "select",         KW_SELECT },
    { "sequence",       SEQUENCE },
    /*    { "static",   KW_STATIC },*/
    { "stop",           STOP },
    { "subroutine",     SUBROUTINE  },
    { "then",           THEN },
    { "to",             KW_TO},
    { "type",           KW_TYPE},
    { "target",         TARGET},
    { "undefined",      KW_UNDEFINED },
    { "use",            KW_USE },
    { "write",          WRITE },
    { "where",          WHERE },
    { 0, 0 }};

struct keyword_token end_keywords[ ] = 
{
    { "block",          KW_BLOCK },
    { "do",             ENDDO },
    { "file",           ENDFILE },
    { "forall",         ENDFORALL },
    { "function",       ENDFUNCTION },
    { "if",             ENDIF },
    { "interface",      ENDINTERFACE },
    { "module",         ENDMODULE },
    { "program",        ENDPROGRAM },
    { "select",         ENDSELECT },
    { "subroutine",     ENDSUBROUTINE },
    { "type",           ENDTYPE },
    { "where",          ENDWHERE },
    { 0, 0 }};

/* EOF */

