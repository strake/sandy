/* A simplified way to customize */
#define USE_TERM_STATUS 0
#define BOTTOM_TITLE    1
#define HILIGHT_CURRENT 0
#define HILIGHT_SYNTAX  1
#define SHOW_NONPRINT   0
#define HANDLE_MOUSE    1

/* Things unlikely to be changed, yet still in the config.h file */
static const bool   isutf8     = TRUE;
static const char   fifobase[] = "/tmp/sandyfifo.";
static       int    tabstop    = 8; /* Not const, as it may be changed via param */
/* static const char   systempath[]  = "/etc/sandy"; */
/* static const char   userpath[]    = ".sandy"; */ /* Relative to $HOME */

#if SHOW_NONPRINT /* TODO: show newline character too (as $) */
static const char   tabstr[3]  = { (char)0xC2, (char)0xBB, 0x00 }; /* Double right arrow */
static const char   spcstr[3]  = { (char)0xC2, (char)0xB7, 0x00 }; /* Middle dot */
static const char   nlstr[2]   = { '$', 0x00 }; /* '$' is tradition for EOL */
#else
static const char   tabstr[2]  = { ' ', 0 };
static const char   spcstr[2]  = { ' ', 0 };
static const char   nlstr[1]   = { 0 };
#endif

/* Helper config functions, not used in main code */
static void f_selOn(const Arg*), f_selOff(const Arg*);
static void f_movebothifsel(const Arg*);
static void f_moveboth(const Arg*);
static void f_pipeai(const Arg*);
static void f_pipeline(const Arg*);
static void f_pipenull(const Arg*);

/* Args to f_spawn */
#define PROMPT(prompt, default, cmd) { .v = (const char *[]){ "/bin/sh", "-c", \
	"if slmenu -v >/dev/null 2>&1; then arg=\"`echo \\\"" default "\\\" | slmenu -t -p '" prompt "'`\";" \
	"else printf \"\033[0;0H\033[7m"prompt"\033[K\033[0m \" >&2; read arg; fi &&" \
	"echo " cmd "\"$arg\" > ${SANDY_FIFO}", NULL } }

#define FIND    PROMPT("Find:",        "${SANDY_FIND}",   "/")
#define FINDBW  PROMPT("Find (back):", "${SANDY_FIND}",   "?")
#define PIPE    PROMPT("Pipe:",        "${SANDY_PIPE}",   "!")
#define SAVEAS  PROMPT("Save as:",     "${SANDY_FILE}",   "w")
#define REPLACE PROMPT("Replace:",     "",                "!echo -n ")
#define SED     PROMPT("Sed:",         "",                "!sed ")
#define EXTPROG PROMPT("Prog+Argu:",   "",                "!")
#define CMD_P   PROMPT("Command:",     "/\n?\nw\nq\n!\nsyntax\noffset\nicase\nro\nai\ndump", "")

/* Args to f_pipe and friends, simple examples are inlined instead */
#define TOCLIP     { .v = "tee /tmp/.sandy.clipboard.$USER | xsel -ib 2>/dev/null" }
#define FROMCLIP   { .v = "xsel -ob 2>/dev/null || cat /tmp/.sandy.clipboard.$USER" }
#define TOSEL      { .v = "tee /tmp/.sandy.selection.$USER | xsel -i 2>/dev/null" }
#define FROMSEL    { .v = "xsel -o 2>/dev/null || cat /tmp/.sandy.selection.$USER" }
#define AUTOINDENT { .v = "awk 'BEGIN{ l=\"\\n\" }; \
				{ if(match($0, \"^[\t ]+[^\t ]\")) l=substr($0, RSTART, RLENGTH-1); \
				  else l=\"\"; \
				  if(FNR==NR && $0 ~ /^[\t ]+$/) print \"\"; \
				  else print }; \
				END{ ORS=\"\"; print l }' 2>/dev/null" }
#define CAPITALIZE { .v = "awk 'BEGIN{ ORS=\"\" }; \
				{ for ( i=1; i <= NF; i++) { $i=tolower($i) ; sub(\".\", substr(toupper($i),1,1) , $i) } \
				if(FNR==NF) print $0; \
				else print $0\"\\n\" }' 2>/dev/null" }

/* Hooks are launched from the main code */
#define HOOK_SAVE_NO_FILE f_spawn (&(const Arg)SAVEAS)
#undef  HOOK_DELETE_ALL   /* This affects every delete */
#undef  HOOK_SELECT_ALL   /* This affects every selection */

/* Key-bindings and stuff */
/* WARNING: use CONTROL(ch) ONLY with '@', (caps)A-Z, '[', '\', ']', '^', '_' or '?' */
/*          otherwise it may not mean what you think. See man 7 ascii for more info */
#define CONTROL(ch)   {(ch ^ 0x40)}
#define META(ch)      { 0x1B, ch }

static const Key curskeys[] = { /* Plain keys here, no CONTROL or META */
/* keyv.i,                  tests,                                  func,            arg        */
{ .keyv.i = KEY_BACKSPACE,  { t_rw,  0,    0,   0 },   (Deed []){ { f_delete,        { .m = m_prevchar } }, { 0 } } },
{ .keyv.i = KEY_DC,         { t_sel, t_rw, 0,   0 },   (Deed []){ { f_delete,        { .m = m_tosel    } }, { 0 } } },
{ .keyv.i = KEY_DC,         { t_rw,  0,    0,   0 },   (Deed []){ { f_delete,        { .m = m_nextchar } }, { 0 } } },
{ .keyv.i = KEY_SDC,        { t_sel, t_rw, 0,   0 },   (Deed []){ { f_delete,        { .m = m_tosel } },    { 0 } } },
{ .keyv.i = KEY_SDC,        { t_rw,  0,    0,   0 },   (Deed []){ { f_delete,        { .m = m_nextchar } }, { 0 } } },
{ .keyv.i = KEY_IC,         { t_sel, 0,    0,   0 },   (Deed []){ { f_pipero,        TOCLIP },              { 0 } } },
{ .keyv.i = KEY_SIC,        { t_rw,  0,    0,   0 },   (Deed []){ { f_pipenull,      FROMCLIP },            { 0 } } },
{ .keyv.i = KEY_HOME,       { t_ai,  0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_smartbol } }, { 0 } } },
{ .keyv.i = KEY_HOME,       { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_bol      } }, { 0 } } },
{ .keyv.i = KEY_END,        { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_eol      } }, { 0 } } },
{ .keyv.i = KEY_SHOME,      { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_bof      } }, { 0 } } },
{ .keyv.i = KEY_SEND,       { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_eof      } }, { 0 } } },
{ .keyv.i = KEY_PPAGE,      { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_prevscr  } }, { 0 } } },
{ .keyv.i = KEY_NPAGE,      { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_nextscr  } }, { 0 } } },
{ .keyv.i = KEY_UP,         { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_prevline } }, { 0 } } },
{ .keyv.i = KEY_DOWN,       { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_nextline } }, { 0 } } },
{ .keyv.i = KEY_LEFT,       { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_prevchar } }, { 0 } } },
{ .keyv.i = KEY_RIGHT,      { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_nextchar } }, { 0 } } },
{ .keyv.i = KEY_SLEFT,      { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_prevword } }, { 0 } } },
{ .keyv.i = KEY_SRIGHT,     { 0,     0,    0,   0 },   (Deed []){ { f_movebothifsel, { .m = m_nextword } }, { 0 } } },
};

static const Key stdkeys[] = {
/* keyv.c,                test,                                  func,        arg */
{ .keyv.c = META(';'),    { t_rw,  0,    0,   0 },  (Deed []){ { f_spawn,     PIPE },                  { 0 } } },
{ .keyv.c = CONTROL('@'), { 0,     0,    0,   0 },  (Deed []){ { f_selOn,     { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('C'), { t_warn,t_mod,0,   0 },  (Deed []){ { f_toggle,    { .i = S_Running } },    { 0 } } },
{ .keyv.c = CONTROL('C'), { t_mod, 0,    0,   0 },  (Deed []){ { f_toggle,    { .i = S_Warned } },     { 0 } } },
{ .keyv.c = CONTROL('C'), { 0,     0,    0,   0 },  (Deed []){ { f_toggle,    { .i = S_Running } },    { 0 } } },
{ .keyv.c = CONTROL('D'), { t_sel, t_rw, 0,   0 },  (Deed []){ { f_pipe,      TOCLIP },                { 0 } } },
{ .keyv.c = CONTROL('G'), { t_sel, 0,    0,   0 },  (Deed []){ { f_selOff,    { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('H'), { t_rw,  0,    0,   0 },  (Deed []){ { f_delete,    { .m = m_prevchar } },   { 0 } } },
{ .keyv.c = CONTROL('I'), { t_rw,  0,    0,   0 },  (Deed []){ { f_insert,    { .v = "\t" } },         { 0 } } },
{ .keyv.c = CONTROL('J'), { t_rw,  t_ai, 0,   0 },  (Deed []){ { f_pipeai,    AUTOINDENT } ,           { 0 } } },
{ .keyv.c = CONTROL('J'), { t_rw,  0,    0,   0 },  (Deed []){ { f_insert,    { .v = "\n" } },         { 0 } } },
{ .keyv.c = CONTROL('K'), { t_eol, t_rw, 0,   0 },  (Deed []){ { f_delete,    { .m = m_nextchar } },   { 0 } } },
{ .keyv.c = CONTROL('K'), { t_rw,  0,    0,   0 },  (Deed []){ { f_delete,    { .m = m_eol } },        { 0 } } },
{ .keyv.c = CONTROL('L'), { 0,     0,    0,   0 },  (Deed []){ { f_center,    { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('M'), { t_rw,  t_ai, 0,   0 },  (Deed []){ { f_pipeai,    AUTOINDENT },            { 0 } } },
{ .keyv.c = CONTROL('M'), { t_rw,  0,    0,   0 },  (Deed []){ { f_insert,    { .v = "\n" } },         { 0 } } },
{ .keyv.c = CONTROL('O'), { t_sel, 0,    0,   0 },  (Deed []){ { f_select,    { .m = m_tosel } },      { 0 } } }, /* Swap fsel and fcur */
{ .keyv.c = CONTROL('R'), { t_sel, 0,    0,   0 },  (Deed []){ { f_findbw,    { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('R'), { 0,     0,    0,   0 },  (Deed []){ { f_spawn,     FINDBW },                { 0 } } },
{ .keyv.c = CONTROL('S'), { t_sel, 0,    0,   0 },  (Deed []){ { f_findfw,    { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('S'), { 0,     0,    0,   0 },  (Deed []){ { f_spawn,     FIND },                  { 0 } } },
//{ .keyv.c = CONTROL('T'), { 0,     0,    0,   0 },  (Deed []){ { f_pipero,    TOCLIP },                { 0 } } },
{ .keyv.c = CONTROL('T'), { t_bol, 0,    0,   0 },  (Deed []){ { 0 } } },
{ .keyv.c = CONTROL('T'), { t_eol, 0,    0,   0 },  (Deed []){ { 0 } } },
{ .keyv.c = CONTROL('T'), { t_rw,  0,    0,   0 },  (Deed []){ { f_swapchars, { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('U'), { t_bol, t_rw, 0,   0 },  (Deed []){ { f_delete,    { .m = m_prevchar } },   { 0 } } },
{ .keyv.c = CONTROL('U'), { t_rw,  0,    0,   0 },  (Deed []){ { f_delete,    { .m = m_bol } },        { 0 } } },
{ .keyv.c = CONTROL('V'), { t_rw,  0,    0,   0 },  (Deed []){ { f_toggle,    { .i = S_InsEsc } },     { 0 } } },
{ .keyv.c = CONTROL('W'), { t_rw,  0,    0,   0 },  (Deed []){ { f_delete,    { .m = m_prevword } },   { 0 } } },
{ .keyv.c = CONTROL('X'), { t_rw,  0,    0,   0 },  (Deed []){ { f_save,      { 0 } },                 { 0 } } },
{ .keyv.c = META('x'),    { 0,     0,    0,   0 },  (Deed []){ { f_spawn,     EXTPROG },               { 0 } } },
{ .keyv.c = CONTROL('Y'), { t_rw,  0,    0,   0 },  (Deed []){ { f_pipenull,  FROMCLIP },              { 0 } } },
{ .keyv.c = CONTROL('Z'), { 0     ,0,    0,   0 },  (Deed []){ { f_suspend,   { 0 } },                 { 0 } } },
{ .keyv.c = CONTROL('^'), { t_redo,t_rw, 0,   0 },  (Deed []){ { f_undo,      { .i = -1 } },           { 0 } } },
{ .keyv.c = CONTROL('^'), { t_rw,  0,    0,   0 },  (Deed []){ { f_repeat,    { 0 } },                 { 0 } } },
{ .keyv.c = META('6'),    { t_rw,  0,    0,   0 },  (Deed []){ { f_pipeline,  { .v = "tr -d '\n'" } }, { 0 } } }, /* Join lines */
{ .keyv.c = META('5'),    { t_sel, t_rw, 0,   0 },  (Deed []){ { f_spawn,     REPLACE },               { 0 } } },
{ .keyv.c = CONTROL('_'), { t_undo,t_rw, 0,   0 },  (Deed []){ { f_undo,      { .i = 1 } },            { 0 } } },
{ .keyv.c = CONTROL('?'), { t_sel, t_rw, 0,   0 },  (Deed []){ { f_delete,    { .m = m_tosel    } },   { 0 } } },
{ .keyv.c = CONTROL('?'), { t_rw,  0,    0,   0 },  (Deed []){ { f_delete,    { .m = m_nextchar } },   { 0 } } },
};

#if HANDLE_MOUSE
/*Mouse clicks */
static const Click clks[] = {
/* mouse mask,           fcur / fsel,      tests,                            func,       arg */
{BUTTON1_CLICKED,        { TRUE , TRUE  }, { 0,     0,     0 }, (Deed []){ { 0,          { 0 } },              { 0 } } },
{BUTTON3_CLICKED,        { TRUE , FALSE }, { t_sel, 0,     0 }, (Deed []){ { f_pipero,   TOSEL },              { 0 } } },
{BUTTON2_CLICKED,        { TRUE , FALSE }, { 0,     0,     0 }, (Deed []){ { f_pipenull, FROMSEL },            { 0 } } },
//{BUTTON4_CLICKED,        { FALSE, FALSE }, { 0,     0,     0 }, (Deed []){ { f_move,     { .m = m_prevscr } }, { 0 } } },
//{BUTTON5_CLICKED,        { FALSE, FALSE }, { 0,     0,     0 }, (Deed []){ { f_move,     { .m = m_nextscr } }, { 0 } } },
/* ^^ NCurses is a sad old library.... it does not include button 5 nor cursor movement in its mouse declaration by default */
{BUTTON1_DOUBLE_CLICKED, { TRUE , TRUE  }, { 0,     0,     0 }, (Deed []){ { f_extsel,   { .i = ExtWord }  },  { 0 } } },
{BUTTON1_TRIPLE_CLICKED, { TRUE , TRUE  }, { 0,     0,     0 }, (Deed []){ { f_extsel,   { .i = ExtLines }  }, { 0 } } },
};
#endif /* HANDLE_MOUSE */

/* Commands read at the fifo */
static const Command cmds[] = { /* REMEMBER: if(arg == 0) arg.v=regex_match */
/* regex,           tests,              func      arg */
{"^([0-9]+)$",      { 0,     0,    0 }, (Deed []){ { f_line ,  { 0 } },                 { 0 } } },
{"^/(.*)$",         { 0,     0,    0 }, (Deed []){ { f_findfw, { 0 } },                 { 0 } } },
{"^\\?(.*)$",       { 0,     0,    0 }, (Deed []){ { f_findbw, { 0 } },                 { 0 } } },
{"^![ \t]*(.*)$",   { t_rw,  0,    0 }, (Deed []){ { f_pipe,   { 0 } },                 { 0 } } },
{"^![ \t]*(.*)$",   { 0,     0,    0 }, (Deed []){ { f_pipero, { 0 } },                 { 0 } } },
{"^w[ \t]*(.*)$",   { t_mod, t_rw, 0 }, (Deed []){ { f_save,   { 0 } },                 { 0 } } },
{"^syntax (.*)$",   { 0,     0,    0 }, (Deed []){ { f_syntax, { 0 } },                 { 0 } } },
{"^offset (.*)$",   { 0,     0,    0 }, (Deed []){ { f_offset, { 0 } },                 { 0 } } },
{"^icase$",         { 0,     0,    0 }, (Deed []){ { f_toggle, { .i = S_CaseIns } },    { 0 } } },
{"^ro$",            { 0,     0,    0 }, (Deed []){ { f_toggle, { .i = S_Readonly } },   { 0 } } },
{"^ai$",            { 0,     0,    0 }, (Deed []){ { f_toggle, { .i = S_AutoIndent } }, { 0 } } },
{"^dump$",          { 0,     0,    0 }, (Deed []){ { f_toggle, { .i = S_DumpStdout } }, { 0 } } },
{"^q$",             { t_mod, 0,    0 }, (Deed []){ { f_toggle, { .i = S_Warned } },     { 0 } } },
{"^q$",             { 0,     0,    0 }, (Deed []){ { f_toggle, { .i = S_Running } },    { 0 } } },
{"^q!$",            { 0,     0,    0 }, (Deed []){ { f_toggle, { .i = S_Running } },    { 0 } } },
};

/* Syntax color definition */
#define B "\\b"
/* #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$"  -- Use this if \b is not in your libc's regex implementation */

static const Syntax syntaxes[] = {
#if HILIGHT_SYNTAX
{"c", "\\.([ch](pp|xx)?|cc)$", {
	/* HiRed   */  "$^",
	/* LoRed   */  "(\"(\\\\.|[^\"])*\")",
	/* HiCyan  */  "$^",
	/* LoCyan  */  "$^",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
	/* LoGreen */  B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|namespace|template|public|protected|private|typename|this|friend|virtual|using|mutable|volatile|register|explicit)"B,
	/* HiMag   */  B"(goto|continue|break|return)"B,
	/* LoMag   */  "(^#(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma))|"B"[A-Z_][0-9A-Z_]+"B"",
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	/* HiYelo  */  "$^",
	/* LoYelo  */  "$^",
	/* HiWhite */  "$^",
	/* LoWhite */  "$^",
	} },
{"sh", "\\.sh$", {
	/* HiRed   */  "$^",
	/* LoRed   */  "\\$\\{?[0-9A-Z_!@#$*?-]+\\}?",
	/* HiCyan  */  "$^",
	/* LoCyan  */  "$^",
	/* HiGreen */  "^[0-9A-Z_]+\\(\\)",
	/* LoGreen */  B"(case|do|done|elif|else|esac|exit|fi|for|function|if|in|local|read|return|select|shift|then|time|until|while)"B,
	/* HiMag   */  "$^",
	/* LoMag   */  "\"(\\\\.|[^\"])*\"",
	/* HiBlue  */  "(\\{|\\}|\\(|\\)|\\;|\\]|\\[|`|\\\\|\\$|<|>|!|=|&|\\|)",
	/* LoBlue  */  "#.*$",
	/* HiYelo  */  "$^",
	/* LoYelo  */  "$^",
	/* HiWhite */  "$^",
	/* LoWhite */  "$^",
	} },

{"makefile", "(Makefile[^/]*|\\.mk)$", {
	/* HiRed   */  "$^",
	/* LoRed   */  "[:=]",
	/* HiCyan  */  "$^",
	/* LoCyan  */  "$^",
	/* HiGreen */  "$^",
	/* LoGreen */  "\\$+[{(][a-zA-Z0-9_-]+[})]",
	/* HiMag   */  B"(if|ifeq|else|endif)"B,
	/* LoMag   */  "$^",
	/* HiBlue  */  "^[^ 	]+:",
	/* LoBlue  */  "#.*$",
	/* HiYelo  */  "$^",
	/* LoYelo  */  "$^",
	/* HiWhite */  "$^",
	/* LoWhite */  "$^",
	} },

{"man", "\\.[1-9]x?$", {
	/* HiRed   */  "\\.(BR?|I[PR]?).*$",
	/* LoRed   */  "$^",
	/* HiCyan  */  "$^",
	/* LoCyan  */  "$^",
	/* HiGreen */  "$^",
	/* LoGreen */  "\\.(S|T)H.*$",
	/* HiMag   */  "\\.(br|DS|RS|RE|PD)",
	/* LoMag   */  "(\\.(S|T)H|\\.TP)",
	/* HiBlue  */  "\\.(BR?|I[PR]?|PP)",
	/* LoBlue  */  "\\\\f[BIPR]",
	/* HiYelo  */  "$^",
	/* LoYelo  */  "$^",
	/* HiWhite */  "$^",
	/* LoWhite */  "$^",
	} },

{"vala", "\\.(vapi|vala)$", {
	/* HiRed   */  B"[A-Z_][0-9A-Z_]+"B,
	/* LoRed   */  "\"(\\\\.|[^\"])*\"",
	/* HiCyan  */  "$^",
	/* LoCyan  */  "$^",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch|get|set|value|out|ref|enum)"B,
	/* LoGreen */  B"(uint|uint8|uint16|uint32|uint64|bool|byte|ssize_t|size_t|char|double|string|float|int|long|short|this|base|transient|void|true|false|null|unowned|owned)"B,
	/* HiMag   */  B"(try|catch|throw|finally|continue|break|return|new|sizeof|signal|delegate)"B,
	/* LoMag   */  B"(abstract|class|final|implements|import|instanceof|interface|using|private|public|static|strictfp|super|throws)"B,
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	/* HiYelo  */  "$^",
	/* LoYelo  */  "$^",
	/* HiWhite */  "$^",
	/* LoWhite */  "$^",
	} },
{"java", "\\.java$", {
	/* HiRed   */  B"[A-Z_][0-9A-Z_]+"B,
	/* LoRed   */  "\"(\\\\.|[^\"])*\"",
	/* HiCyan  */  "^$",
	/* LoCyan  */  "^$",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch)"B,
	/* LoGreen */  B"(boolean|byte|char|double|float|int|long|short|transient|void|true|false|null)"B,
	/* HiMag   */  B"(try|catch|throw|finally|continue|break|return|new)"B,
	/* LoMag   */  B"(abstract|class|extends|final|implements|import|instanceof|interface|native|package|private|protected|public|static|strictfp|this|super|synchronized|throws|volatile)"B,
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	/* HiYelo  */  "^$",
	/* LoYelo  */  "^$",
	/* HiWhite */  "^$",
	/* LoWhite */  "^$",
	} },
{"ruby", "\\.rb$", {
	/* HiRed   */  "(\\$|@|@@)?"B"[A-Z]+[0-9A-Z_a-z]*",
	/* LoRed   */  "#\\{[^}]*\\}",
	/* HiCyan  */  "^$",
	/* LoCyan  */  "^$",
	/* HiGreen */  B"(__FILE__|__LINE__|BEGIN|END|alias|and|begin|break|case|class|def|defined\?|do|else|elsif|end|ensure|false|for|if|in|module|next|nil|not|or|redo|rescue|retry|return|self|super|then|true|undef|unless|until|when|while|yield)"B,
	/* LoGreen */  "([ 	]|^):[0-9A-Z_]+"B,
	/* HiMag   */  "(/([^/]|(\\/))*/[iomx]*|%r\\{([^}]|(\\}))*\\}[iomx]*)",
	/* LoMag   */  "(`[^`]*`|%x\\{[^}]*\\})",
	/* HiBlue  */  "(\"([^\"]|(\\\\\"))*\"|%[QW]?\\{[^}]*\\}|%[QW]?\\([^)]*\\)|%[QW]?<[^>]*>|%[QW]?\\[[^]]*\\]|%[QW]?\\$[^$]*\\$|%[QW]?\\^[^^]*\\^|%[QW]?![^!]*!|\'([^\']|(\\\\\'))*\'|%[qw]\\{[^}]*\\}|%[qw]\\([^)]*\\)|%[qw]<[^>]*>|%[qw]\\[[^]]*\\]|%[qw]\\$[^$]*\\$|%[qw]\\^[^^]*\\^|%[qw]![^!]*!)",
	/* LoBlue  */  "(#[^{].*$|#$)",
	/* HiYelo  */  "^$",
	/* LoYelo  */  "^$",
	/* HiWhite */  "^$",
	/* LoWhite */  "^$",
	} },
#else  /* HILIGHT_SYNTAX */
{"", "\0", { "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0" } }
#endif /* HILIGHT_SYNTAX */
};

/* Colors */
static const short  fgcolors[LastFG] = {
	[DefFG]   = -1,
	[CurFG]   = (HILIGHT_CURRENT?COLOR_BLACK:-1),
	[SelFG]   = COLOR_BLACK,
	[SpcFG]   = COLOR_WHITE,
	[CtrlFG]  = COLOR_RED,
	[Syn0FG]  = COLOR_RED,
	[Syn1FG]  = COLOR_RED,
	[Syn2FG]  = COLOR_CYAN,
	[Syn3FG]  = COLOR_CYAN,
	[Syn4FG]  = COLOR_GREEN,
	[Syn5FG]  = COLOR_GREEN,
	[Syn6FG]  = COLOR_MAGENTA,
	[Syn7FG]  = COLOR_MAGENTA,
	[Syn8FG]  = COLOR_BLUE,
	[Syn9FG]  = COLOR_BLUE,
	[Syn10FG] = COLOR_YELLOW,
	[Syn11FG] = COLOR_YELLOW,
	[Syn12FG] = COLOR_WHITE,
	[Syn13FG] = COLOR_WHITE,
};

static const int colorattrs[LastFG] = {
	[DefFG]   = 0,
	[CurFG]   = 0,
	[SelFG]   = 0,
	[SpcFG]   = A_DIM,
	[CtrlFG]  = A_DIM,
	[Syn0FG]  = A_BOLD,
	[Syn1FG]  = 0,
	[Syn2FG]  = A_BOLD,
	[Syn3FG]  = 0,
	[Syn4FG]  = A_BOLD,
	[Syn5FG]  = 0,
	[Syn6FG]  = A_BOLD,
	[Syn7FG]  = 0,
	[Syn8FG]  = A_BOLD,
	[Syn9FG]  = 0,
	[Syn10FG] = A_BOLD,
	[Syn11FG] = 0,
	[Syn12FG] = A_BOLD,
	[Syn13FG] = 0,
};

static const int bwattrs[LastFG] = {
	[DefFG]   = 0,
	[CurFG]   = 0,
	[SelFG]   = A_REVERSE,
	[SpcFG]   = A_DIM,
	[CtrlFG]  = A_DIM,
	[Syn0FG]  = A_BOLD,
	[Syn1FG]  = A_BOLD,
	[Syn2FG]  = A_BOLD,
	[Syn3FG]  = A_BOLD,
	[Syn4FG]  = A_BOLD,
	[Syn5FG]  = A_BOLD,
	[Syn6FG]  = A_BOLD,
	[Syn7FG]  = A_BOLD,
	[Syn8FG]  = A_BOLD,
	[Syn9FG]  = A_BOLD,
	[Syn10FG] = A_BOLD,
	[Syn11FG] = A_BOLD,
	[Syn12FG] = A_BOLD,
	[Syn13FG] = A_BOLD,
};

static const short  bgcolors[LastBG] = {
	[DefBG] = -1,
	[CurBG] = (HILIGHT_CURRENT?COLOR_WHITE:-1),
	[SelBG] = COLOR_WHITE,
};

void
f_selOn (const Arg *arg) {
	f_select (&(Arg){ .m = m_stay });
	selmode = 1;
}

void
f_selOff (const Arg *arg) {
	selmode = 0;
}

void
f_movebothifsel (const Arg *arg) {
	(selmode ? f_move : f_moveboth) (arg);
}

/* Helper config functions implementation */
void /* Move both cursor and selection point, thus cancelling the selection */
f_moveboth(const Arg *arg) {
	fsel=fcur=arg->m(fcur);
}

void /* Pipe selection from bol, then select last line only, special for autoindenting */
f_pipeai(const Arg *arg) {
	i_sortpos(&fsel, &fcur);
	fsel.o=0;
	f_pipe(arg);
	fsel.l=fcur.l; fsel.o=0;
}

void /* Pipe full lines including the selection */
f_pipeline(const Arg *arg) {
	f_extsel(&(const Arg){ .i = ExtLines });
	f_pipe(arg);
}

void /* Pipe empty text */
f_pipenull(const Arg *arg) {
	fsel=fcur;
	f_pipe(arg);
}
