#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct {
    char name[50];
    char value[100];
} Ident;

typedef enum {
    ASSIGN_OP, ADD_OP, MULT_OP, SEMICOLON, IDENT, CONSTANT, END_OF_FILE, LPAREN, RPAREN, UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char token_string[50];
} Token;

char line[500], error_ident[50];
FILE *file;
Ident idArray[50];
Token tokenArray[200];

void printResultByLine(char *line, int ID, int CON, int OP);
void printIdent(int num_ident);
void parse();
void parse_V();

void printOK(void);
void printToken(char *token);
void printOPWarning(int code);
void printIDError(char *name);

int isInt(const char *str);
void statements();
void statement();
char *expression(char *result);
char *term_tail(char *inherited_value, char *result);
char *term(char *result);
char *factor(char *result);
char *factor_tail(char *inherited_value, char *result);

void getChar();
void addChar();
void getNonBlank();
void lexical();
char *lookupIdentValue(char *name);
void setIdentValue(char *name, char *value);

int add_pos;
int get_pos;
int push_pos;

int id_cnt;
int const_cnt;
int op_cnt;

int id_tot;
int id_error;
int id_warning;
int vflag;

char current_char;
char token_string[100];

Token next_token;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-v] <filepath>\n", argv[0]);
        return 1;
    }

    int verbose = 0;
    char *filepath = NULL;

    if (strcmp("-v", argv[1]) == 0) {
        verbose = 1;
        if (argc < 3) {
            fprintf(stderr, "Error: No file specified.\n");
            return 1;
        }
        filepath = argv[2];
    } else {
        filepath = argv[1];
    }

    file = fopen(filepath, "r");

    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filepath);
        return 1;
    }

    fgets(line, sizeof(line), file);

    if (verbose) {
        parse_V();
    } else {
        parse();
    }

    fclose(file);
    return 0;
}

void parse() {
    id_cnt = const_cnt = op_cnt = 0;
    get_pos = push_pos = 0;
    id_error = id_warning = 0;

    while (line[0] != '\0') {
        while (strchr(line, ';') == NULL) {
            char temp_line[100];
            if (!fgets(temp_line, sizeof(temp_line), file)) {
                break;
            }
            strcat(line, temp_line);
        }

        char *statement = strtok(line, ";");
        while (statement != NULL) {
            while (isspace(*statement)) statement++;
            char *end = statement + strlen(statement) - 1;
            while (end > statement && isspace(*end)) end--;
            *(end + 1) = '\0';

            if (strlen(statement) == 0) {
                statement = strtok(NULL, ";");
                continue;
            }

            strcpy(line, statement);
            strcat(line, ";"); 

            getChar();
            while (current_char != '\0') {
                lexical();
                statements();
            }

            char fline[500] = "";
            for (int i = 0; i < push_pos; i++) {
                const char *current = tokenArray[i].token_string;
                strcat(fline, current);
                if (i < push_pos - 1) {
                    const char *next = tokenArray[i + 1].token_string;
                    if (!strcmp(current, "(") || !strcmp(next, ")") || !strcmp(next, ";")) continue;
                    strcat(fline, " ");
                }
            }

            printResultByLine(fline, id_cnt, const_cnt, op_cnt);
            if (!id_error && !id_warning) {
                printOK();
            } else if (id_warning > 0 && !id_error) {
                printOPWarning(id_warning);
            } else if (id_error) {
                printIDError(error_ident);
            }

            id_cnt = const_cnt = op_cnt = 0;
            id_error = id_warning = 0;
            push_pos = 0;

            statement = strtok(NULL, ";");
            get_pos = 0;
        }

        line[0] = '\0';
        if (!fgets(line, sizeof(line), file)) {
            break;
        }
    }

    printIdent(id_tot);
}



void parse_V() {
    id_cnt = const_cnt = op_cnt = 0;
    get_pos = add_pos = push_pos = 0;
    id_error = id_warning = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        while (strchr(line, ';') == NULL) {
            char temp_line[100];
            if (!fgets(temp_line, sizeof(temp_line), file)) {
                break;
            }
            temp_line[strcspn(temp_line, "\n")] = '\0';
            strcat(line, temp_line);
        }

        char *statement = strtok(line, ";");
        while (statement != NULL) {
            strcpy(line, statement);
            strcat(line, ";");  

            getChar();
            while (current_char != '\0') {
                add_pos = 0;
                lexical();
                if (next_token.type != UNKNOWN) {
                    switch (next_token.type) {
                        case ASSIGN_OP:
                            printToken("assignment_op"); break;
                        case ADD_OP:
                            printToken("add_operator"); break;
                        case MULT_OP:
                            printToken("mult_operator"); break;
                        case SEMICOLON:
                            printToken("semi_colon");
                            char fline[500] = "";
                            for (int i = 0; i < push_pos; i++) {
                                const char *current = tokenArray[i].token_string;
                                strcat(fline, current);
                                if (i < push_pos - 1) {
                                    const char *next = tokenArray[i + 1].token_string;
                                    if (!strcmp(current, "(") || !strcmp(next, ")") || !strcmp(next, ";")) continue;
                                    strcat(fline, " ");
                                }
                            }
                            if (strlen(fline) > 0) {
                                strcpy(line, fline);
                            }
                            printResultByLine(line, id_cnt, const_cnt, op_cnt);
                            if (!id_error && !id_warning) printOK();
                            else if (id_warning > 0 && !id_error) printOPWarning(id_warning);
                            else printIDError(error_ident);

                            id_cnt = const_cnt = op_cnt = 0;
                            push_pos = 0;
                            break;
                        case IDENT:
                            printToken("ident"); break;
                        case CONSTANT:
                            printToken("const"); break;
                        case LPAREN:
                            printToken("left_paren"); break;
                        case RPAREN:
                            printToken("right_paren"); break;
                    }
                }
            }

            statement = strtok(NULL, ";");
        }
    }

    printIdent(id_tot);
}


void printResultByLine(char *line, int ID, int CON, int OP) {
    printf("%s\n", line);
    printf("ID: %d; CONST: %d; OP: %d;\n", ID, CON, OP);
}

void printOPWarning(int code){
	switch(code){
	
    	case 1:
    		printf("(Warning) \"Eliminating duplicate operator (+)\"\n");
    		break;
    	case 2:
    		printf("(Warning) \"Eliminating duplicate operator (-)\"\n");
    		break;
    	case 3:
    		printf("(Warning) \"Eliminating duplicate operator (*)\"\n");
    		break;
    	case 4:
    		printf("(Warning) \"Eliminating duplicate operator (/)\"\n");
    		break;
    	case 5:
    		printf("(Warning) \"Substituting assignment operator (:=)\"\n"); 
    		break;
    	}
}

void printOK(){
	printf("(OK)\n");
}

void printIDError(char *name){
	printf("(Error) \"referring to undefined identifiers(%s)\"\n",name);
}

void printIdent(int num_ident) {
    int i;
    printf("Result ==>");
    for (i = 0; i < num_ident; i++) {
        printf(" %s: %s;", idArray[i].name, idArray[i].value);
    }
}

void printToken(char *token){
	printf("%s\n", token);
}

void statements() {
    statement();
    if (next_token.type == SEMICOLON || next_token.type == END_OF_FILE) {
        lexical();
        statements();
    }
}

void statement() {
    char ident_name[50];

    if (next_token.type == IDENT) {
        strcpy(ident_name, next_token.token_string);

        lexical();

        if (next_token.type == ASSIGN_OP) {
            char expr_value[50] = "";
            lexical();
            expression(expr_value);
            setIdentValue(ident_name, expr_value);
        }
    }
}

char *expression(char *result) {
    char term_result[50] = "";
    term(term_result);
    term_tail(term_result, result);
    return result;
}

char *term_tail(char *inherited_value, char *result) {
    if (next_token.type == ADD_OP) {
        char term_result[50] = "";
        char operator = next_token.token_string[0];
        lexical();

        term(term_result);
        if (strcmp(inherited_value, "Unknown") && strcmp(term_result, "Unknown")) {
            int answer = (operator == '+') ? atoi(inherited_value) + atoi(term_result)
                                           : atoi(inherited_value) - atoi(term_result);
            sprintf(result, "%d", answer);
        } else {
            strcpy(result, "Unknown");
        }
        return term_tail(result, result);  
    }
    if (result != inherited_value)
        strcpy(result, inherited_value);
    return result;
}

char *term(char *result) {
    char factor_result[50] = "";
    factor(factor_result); 
    factor_tail(factor_result, result);  
    return result;
}

char *factor_tail(char *inherited_value, char *result) {
    if (next_token.type == MULT_OP) {
        char factor_result[50] = "";
        char operator = next_token.token_string[0];
        lexical();

        factor(factor_result);  
        if (strcmp(inherited_value, "Unknown") && strcmp(factor_result, "Unknown")) {
            int answer = (operator == '*') ? atoi(inherited_value) * atoi(factor_result)
                                           : atoi(inherited_value) / atoi(factor_result);
            sprintf(result, "%d", answer);
        } else {
            strcpy(result, "Unknown");
        }
        return factor_tail(result, result); 
    }
    if (result != inherited_value)
        strcpy(result, inherited_value);
    return result;
}

char *factor(char *result) {
    if (next_token.type == LPAREN) {
        lexical();  
        expression(result); 
        if (next_token.type == RPAREN) {
            lexical();  
        }
    } else if (next_token.type == IDENT) {
        char *found_val = lookupIdentValue(next_token.token_string);
        if (!found_val) {
            strcpy(result, "Unknown");
        } else {
            strcpy(result, found_val);
        }
        lexical();
    } else if (next_token.type == CONSTANT) {
        strcpy(result, next_token.token_string);
        lexical();
    }
    return result;
}

char *lookupIdentValue(char *name) {
    for (int i = 0; i < id_tot; i++) {
        if (!strcmp(idArray[i].name, name)) {
            return idArray[i].value;
        }
    }
    setIdentValue(name, "Unknown");
    id_error = 1;
    strcpy(error_ident, name);
    return NULL;
}

void setIdentValue(char *name, char *value) {
    for (int i = 0; i < id_tot; i++) {
        if (!strcmp(idArray[i].name, name)) {
            strcpy(idArray[i].value, value);
            return;
        }
    }
    strcpy(idArray[id_tot].name, name);
    strcpy(idArray[id_tot++].value, value);
}

void getChar() {
    if (line[get_pos] != '\0') {
        current_char = line[get_pos++];
    } else {
        current_char = '\0';
    }
}

void addChar() {
    if (add_pos < 99) {
        token_string[add_pos++] = current_char;
        token_string[add_pos] = '\0';
    }
}

void getNonBlank() {
    while (((int)current_char < 33 && current_char) || isspace(current_char)) {
        getChar();
    }
}

Token new_token(TokenType type, char *token_string) {
    Token token;
    token.type = type;
    strncpy(token.token_string, token_string, sizeof(token.token_string) - 1);
    token.token_string[sizeof(token.token_string) - 1] = '\0';
    return token;
}

void lexical() {
    add_pos = 0;
    getNonBlank();
    
    if (isdigit(current_char)) {
        while (isdigit(current_char)) {
            addChar();
            getChar();
        }
        const_cnt++;
        next_token = new_token(CONSTANT, token_string);
        tokenArray[push_pos++] = next_token;
    }
    
    // IDENT
    else if (isalpha(current_char)) {
        while (isalnum(current_char)) {
            addChar();
            getChar();
        }
        id_cnt++;
        next_token = new_token(IDENT, token_string);
        tokenArray[push_pos++] = next_token;
    }
    
    // LPAREN
    else if (current_char == '(') {
        getChar();
        next_token = new_token(LPAREN, "(");
        tokenArray[push_pos++] = next_token;
    }
    
    // RPAREN
    else if (current_char == ')') {
        getChar();
        next_token = new_token(RPAREN, ")");
        tokenArray[push_pos++] = next_token;
    }
    
    // SEMICOLON
    else if (current_char == ';') {
        getChar();
        next_token = new_token(SEMICOLON, ";");
        tokenArray[push_pos++] = next_token;
    }
    
    // ADD_OP
    else if (current_char == '+') {
        getChar();
        getNonBlank();
        if (current_char == '+') {
            id_warning = 1;
            getChar();
        }
        op_cnt++;
        next_token = new_token(ADD_OP, "+");
        tokenArray[push_pos++] = next_token;
    }
    
    else if (current_char == '-') {
        getChar();
        getNonBlank();
        if (current_char == '-') {
            id_warning = 2;
            getChar();
        }
        op_cnt++;
        next_token = new_token(ADD_OP, "-");
        tokenArray[push_pos++] = next_token;
    }
    
    // MULT_OP
    else if (current_char == '*') {
        getChar();
        getNonBlank();
        if (current_char == '*') {
            id_warning = 3;
            getChar();
        }
        op_cnt++;
        next_token = new_token(MULT_OP, "*");
        tokenArray[push_pos++] = next_token;
    }
    
    else if (current_char == '/') {
        getChar();
        getNonBlank();
        if (current_char == '/') {
            id_warning = 4;
            getChar();
        }
        op_cnt++;
        next_token = new_token(ADD_OP, "/");
        tokenArray[push_pos++] = next_token;
    }
    
    // ASSIGN_OP
    else if (current_char == '=' || current_char == ':') {
        if (current_char == '=') {
            getChar();
            id_warning = 5;
            next_token = new_token(ASSIGN_OP, ":=");
            tokenArray[push_pos++] = next_token;
        }
        else if (current_char == ':') {
            getChar();
            if (current_char == '=') {
                getChar();
                next_token = new_token(ASSIGN_OP, ":=");
                tokenArray[push_pos++] = next_token;
            }
        }
    }
    
    // UNKNOWN
    else {
        while (current_char > 32 && current_char != ' ') {
            addChar();
            getChar();
        }
        next_token = new_token(UNKNOWN, token_string);
    }
}
