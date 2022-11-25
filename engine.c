#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

typedef struct {
    char* name;
    char** attribute_type;
    char** attributes;
    int* att_sizes;
    int row_size;
    int att_num;
    int num_rows;
    bool containsVar;
    char* primary_key;
    char* primary_keys;
    void* root;
    void* cursor;
    struct Table* next; 
} Table;

typedef struct {
    Table* head;
    Table* tail;
    int size;
    struct DP_primary* next;
} DP_primary;

DP_primary * dp_prim;
DP_primary * dp_prim_root;
const int SIZE = 500;

/**
 * Returns the size of the corresponding data type
 */
int find_size(char* input) {
    if(strcmp(input, "smallint") == 0) {
        return 8;
    } else if(strcmp(input, "integer") == 0) {
        return 16;                                       
    } else if(strcmp(input, "real") == 0) {
        return 4;                                       
    } else if(strncmp(input, "char", 4) == 0) {
        char* size = malloc((strlen(input) - 6) * sizeof(char));
        int ret;

        strncpy(size, input+5, 2);
        ret = atoi(size);
        free(size);
        return ret;
    }
}

/**
 * Creates a new table with the given variables 
 */
Table* create_table(const char *table_name, const char *key, int length, ...) {
    
    va_list arg_list;
    va_start(arg_list, length); 

    Table* table = malloc(sizeof(Table));
    table->name = table_name;
    table->next = NULL;
    table->root = malloc(256 * sizeof(void));
    table->cursor = table->root;
    table->attribute_type = malloc(length/2 * (sizeof(char*)));
    table->attributes = malloc(length/2 * sizeof(char*));
    table->att_sizes = malloc(length/2 * sizeof(int));
    table->primary_key = key;
    table->primary_keys = malloc(80 * sizeof(char));
    table->att_num = length/2;
    table->num_rows = 0;
    table->row_size = 0;
    table->containsVar = false;

    //if the current dp_prim is full, create a new one.
    if(dp_prim->head == NULL) {
        dp_prim->head = table;
        dp_prim->tail = table;
        dp_prim->size = 0;
    } else if(dp_prim->size < SIZE) { 
        dp_prim->tail->next = table;
        dp_prim->tail = table; 
    } else if (dp_prim->size >= SIZE) {
        DP_primary* new_dp = malloc(sizeof(DP_primary));
        new_dp->head = table;
        new_dp->tail = table;
        new_dp->size = 0;
        dp_prim->next = new_dp;
        dp_prim = dp_prim->next;
    }

    printf ("CREATE TABLE %s \n", table_name);
    
    char* temp;
    for(int cntr = 0; cntr < length / 2; cntr++) {
        table->attributes[cntr] = malloc(20 * sizeof(char));       
        table->attribute_type[cntr] = malloc(10 * sizeof(char));

        temp = va_arg(arg_list, char*);        
        strcpy(table->attributes[cntr], temp);
        printf(" %-10s ", temp);

        temp = va_arg(arg_list, char*);        
        strcpy(table->attribute_type[cntr], temp);
        table->row_size += find_size(temp);
        printf(" %s,\n", temp);

        if(strncmp(temp, "varchar", 7) == 0) {
            table->containsVar = true;
        }
    }

    printf (" PRIMARY KEY (%s)\n", key);
    printf (");\n");
    va_end(arg_list);

    return table;
}

/**
 * Given a table name, it finds the table with the specified name,
 * returns a pointer to that table.
 * If a table with a given name doesn't exits, NULL pointer is returned. 
 */
Table* find_table (char* table_name) {
    DP_primary* dp_curr = dp_prim_root;

    while(dp_curr != NULL) {
        Table* curr = dp_curr->head;

        while (curr != NULL) {
            if(strcmp(curr->name, table_name) == 0) {
                return curr;
            }
            curr = curr->next;
        }

        dp_curr = dp_curr->next;
    }

    return NULL;
}

/**
 * Prints the information in the row starting at ptr
 */
void print_row(Table* table, void* ptr, void** temp) {
    for(int i = 0; i < table->att_num; i++) {
        if(strcmp(table->attribute_type[i], "smallint") == 0) {
            printf("%d ", *((int*)ptr));
            ptr += find_size(table->attribute_type[i]);
        } 
        else if(strcmp(table->attribute_type[i], "integer") == 0) {
            printf("%d ", *((int*)ptr));
            ptr += find_size(table->attribute_type[i]);
        } 
        else if (strncmp(table->attribute_type[i], "char", 4) == 0) {
            char* str = (char*)ptr;
            printf("%s ", str);
            ptr += find_size(table->attribute_type[i]) * sizeof(char);
        }
        else if (strncmp(table->attribute_type[i], "varchar", 7) == 0) {
            int len = *((int*)ptr);
            ptr += sizeof(int); 
            char* str = malloc(len * sizeof(char));
            strncpy(str, (char*)ptr, len);
            printf("%s ", str);
            ptr += len * sizeof(char);
            free(str);
        }
    }
    *temp = ptr;
    printf("\n");
}

/**
 * Prints the information in the specified table
 */
void print_table(Table* table) {
    void *ptr = table->root;
    void** temp = &ptr;
    printf("\nTable: %s\n", table->name);
    int row_cnt = 1;
    
    while(row_cnt <= table->num_rows) {
        print_row(table, ptr, temp);      
        ptr = *temp;
        row_cnt++;
    }
}

/**
 * Checks if an attribute is the primary key and 
 * if so, checks if the value at the temp pointer already exists
 * as a primary key and can't be added or updated.
 */
bool isPrimaryKeyExists(Table* table, int i, void* temp) {
    if(strcmp(table->attributes[i], table->primary_key) == 0) {
        if(strstr(table->primary_keys, (char*)temp) == NULL) {
            strcat(table->primary_keys, (char*)temp);
            strcat(table->primary_keys, ", ");
            return false;
        } else {
            printf("\nNo primary key (%s) can be repeated. Please try again!\n", table->attributes[i]);
            return true;
        }
    }

    return false;
}

/**
 * Inserts the values to the table
 */
void insert(char *table_name, int length, ...) {
    va_list arg_list;
    va_start(arg_list, length);

    Table* table = find_table(table_name);
    if(table == NULL) {
        printf("The table named %s does not exist! \n", table_name);
        return;
    }

    printf ("\nINSERT INTO %s VALUES (", table_name);
    void *temp = table->cursor;
        
    for(int i = 0; i < length; i++) {
        if(strcmp(table->attribute_type[i], "smallint") == 0) {

            int val = va_arg(arg_list, int);
            *((int *)temp) = val;
            if(isPrimaryKeyExists(table, i, temp)) return;

            temp += 8;
            dp_prim->size += 8;
            table->att_sizes[i] = 8;
            printf("%d", val);                 

        } else if(strcmp(table->attribute_type[i], "integer") == 0) {

            int val = va_arg(arg_list, int);
            *((int *)temp) = val;
            if(isPrimaryKeyExists(table, i, temp)) return;

            temp += 16;
            dp_prim->size += 16;
            table->att_sizes[i] = 16;
            printf("%d", val);                               

        } else if (strncmp(table->attribute_type[i], "char", 4) == 0) {
        
            char* val = va_arg(arg_list, char*);
            strncpy((char*)temp, val, strlen(val));
            if(isPrimaryKeyExists(table, i, temp)) return;

            int size = find_size(table->attribute_type[i]);
            temp += size * sizeof(char);
            dp_prim->size += size;
            table->att_sizes[i] = size;
            printf("’%s’", val);

        } else if (strncmp(table->attribute_type[i], "varchar", 7) == 0) { 
            
            char* val = va_arg(arg_list, char*);
            int len = strlen(val);
            *((int *)temp) = len;
            temp += sizeof(int);
            dp_prim->size += 32;
            strncpy((char*)temp, val, len);
            printf("’%s’", (char*)temp);
            temp += len * sizeof(char);
            dp_prim->size += len;
        }

        if(i < length-1) {
            printf(", ");
        }
    }
    printf (");\n");
    print_row(table, table->cursor, &(table->cursor));
    
    table->cursor = temp;
    table->num_rows++;
}

/**
 * Given the attribute name, it goes through the attribute list and
 * returns the location of the specified attribute in the first row.
 */
void* find_attribute_loc(Table* table, char* att_name) {
    void* temp = table->root;
    
    for(int i = 0; i < table->att_num; i++) {
        if(strcmp(att_name, table->attributes[i]) == 0) {
            return temp;
        }
        temp += table->att_sizes[i];
    }
    return temp;
}

/**
 * Checks if the specified row with row_num qualifies for the given condition
 * If the row qualifies the condition, the starting loc of the row is returned.
 * The condition is (att_name value) + sign(=, !=, <, >) + value
 */                                                                                                 
bool is_row(Table* table, char* att_name, int value, char* sign, int row_num, void* temp) { 
    if(strcmp(sign, "=") == 0 && *((int*)temp) == value) return true;
    else if(strcmp(sign, "!=") == 0 && *((int*)temp) != value) return true;
    else if(strcmp(sign, ">") == 0 && *((int*)temp) > value) return true;
    else if(strcmp(sign, "<") == 0 && *((int*)temp) < value) return true;
    else if(strcmp(sign, " ") == 0) return true;

    return false;
}

/**
 * Checks if the attribute in the row satisfies the given condition
 */
bool check_condition_var(Table* table, char* att_name, int filter, char* sign, int num_row, void* ptr) {
    void* temp = ptr;

    for(int i = 0; i < table->att_num; i++) {

        if(strcmp(att_name, table->attributes[i]) == 0 && is_row(table, att_name, filter, sign, num_row, temp)) {
            return true;
        }

        if(strncmp(table->attribute_type[i], "varchar", 7) == 0) {
            int len = *((int*)temp);
            temp += sizeof(int);  
            temp += len;
        } 
        else {
            temp += table->att_sizes[i];
        }
    }

    return false;
}

/**
 * Executes the update method
 */
void update_with_varchar(Table* table, char* att_name, int filter, char* sign,  int new_int, char* new_str, char* to_update) {
    void* ptr = table->root;
    int cntr = 0;

    for(int i = 0; i < table->num_rows; i++) {
        bool condition_checked = check_condition_var(table, att_name, filter, sign, i, ptr);

        for(int k = 0; k < table->att_num; k++) {
            if(strcmp(table->attributes[k], to_update) == 0 && condition_checked) {

                if (new_int != NULL) {
                    if(isPrimaryKeyExists(table, k, &new_int)) return;
                    *((int*)(ptr)) = new_int;
                    ptr += find_size(table->attribute_type[k]);
                } 
                else if (new_str != NULL && strncmp(table->attribute_type[k], "char", 4) == 0) {
                    if(isPrimaryKeyExists(table, k, &new_str)) return;
                    
                    strcpy((char*)(ptr), new_str);
                    ptr += find_size(table->attribute_type[k]);
                }
                else if (new_str != NULL && strncmp(table->attribute_type[k], "varchar", 7) == 0) {
                    if(isPrimaryKeyExists(table, k, &new_str)) return;
                    int len = *((int*)ptr);
                    ptr += sizeof(int);
                    char* str = (char*)ptr;
                    strcpy((char*)(ptr), new_str);
                    ptr += len;
                }
            
            } else {
                if(strncmp(table->attribute_type[k], "varchar", 7) == 0) {
                    int len = *((int*)ptr);
                    ptr += sizeof(int);
                    ptr += len;
                } else {
                    ptr += table->att_sizes[k];
                }
            }
        }

        cntr++;
    }
}

/**
 * Executes the update method if the table doesn't include varchar type
 */
void execute_update(Table* table, char* att_name, int filter, char* sign, int offset,  int new_int, char* new_str) {
    for(int i = 0; i < table->num_rows; i++) {
        void* temp = find_attribute_loc(table, att_name);
        temp = temp + (i * (table->row_size));

        if(is_row(table, att_name, filter, sign, i, temp) == true) {
            void* loc = table->root + i * table->row_size;

            if (new_int != NULL) {
                *((int*)(loc + offset)) = new_int;                     
            } else if (new_str != NULL) {
                strcpy((char*)(loc+offset), new_str);
            }
        }
    }
}

/**
 * Reads in the input for the update function
 */
void update(const char *table_name, int length, ...) {
    va_list arg_list;
    va_start(arg_list, length);

    Table* table = find_table(table_name);
    if(table == NULL) {
        printf("The table named %s does not exist! \n", table_name);
        return;
    }

    char* temp = va_arg(arg_list, char*);
    char* data_type;
    int offset;
    int new_int = NULL;
    char* new_str = NULL;
    printf ("\nUPDATE %s SET ", table_name);
    printf ("%s = ", temp);

    offset = 0;
    for(int i = 0; i < table->att_num; i++) {
        
        if(strcmp(temp, table->attributes[i]) == 0) {

            data_type = table->attribute_type[i];

            if(strcmp(data_type, "smallint") == 0 || strcmp(data_type, "integer") == 0) {
                new_int = va_arg(arg_list, int);
                printf ("%d ", new_int);
                break;
            } else if(strncmp(data_type, "char", 4) == 0) {
                new_str = va_arg(arg_list, char*);
                printf ("%s ", new_str);
                break;
            } else if (strncmp(data_type, "varchar", 7) == 0) {
                new_str = va_arg(arg_list, char*);
                printf ("%s ", new_str);
                break;
            }
        }

        offset += table->att_sizes[i];
    }

    char* condition = va_arg(arg_list, char*);
    char* end = strdup(condition);
    char* att = strsep(&end, " ");
    char* symbol = strsep(&end, " ");
    int value = atoi(strsep(&end, " "));

    printf ("WHERE %s;\n", condition);

    if(table->containsVar) {
        update_with_varchar(table, att, value, symbol, new_int, new_str, temp);
    } else {
        execute_update(table, att, value, symbol, offset, new_int, new_str);
    }

    print_table(table);
}

/**
 * Given the attribute name, it finds the data type correponding to that attribute and
 * returns the data type as a string
 */
char* find_data_type(Table* table, char *att_name, int* offset) {
    int temp = 0;

    for(int i = 0; i < table->att_num; i++) {
        if(strcmp(att_name, table->attributes[i]) == 0) {
            *offset = temp;
            return table->attribute_type[i];
        }

        temp += table->att_sizes[i];            
    }
    return "";
}

/**
 * Separates the select input by a comma and 
 * counts the number of attributes selected
 */
char** prepare_select_statement(char* input, int* length) {

    char** list = (char**) malloc (20 * sizeof (char*));
    char** temp = list;
    char* str;
    char* end = strdup(input);

    while((str = strsep(&end, ", ")) != NULL) {
        *temp = (char*) malloc (strlen(str) * sizeof(char));
        *temp = str;
        temp++;
        (*length)++;
    }
    return list;
}
/**
 * Checks if an attribute is in the select statement
 */
bool select_contains(char** list, char* type, int length) {
    for(int i = 0; i < length; i++) {
        if(strcmp(list[i], type) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Increments the pointer by the corresponding size of the data type.
 */
void incr_pointer(Table* table, int k, void* ptr, void** ret) {
    if(strncmp(table->attribute_type[k], "char", 4) == 0) {
        ptr += table->att_sizes[k];
        
    } else if(strcmp(table->attribute_type[k], "smallint") == 0) {
        ptr += 8;
    } else if(strcmp(table->attribute_type[k], "integer") == 0) {
        ptr += 16;
    } else if(strncmp(table->attribute_type[k], "varchar", 7) == 0) {
        int len = *((int*)ptr);
        ptr += sizeof(int);
        ptr += len;
    }

    *ret = ptr;
}

/**
 * Executes the select statement
 */
void select_with_varchar(Table* table, char* att_name, int filter, char* sign, char** input, int length) {
    void* ptr = table->root;
    
    for(int i = 0; i < table->num_rows; i++) {
        
        bool condition_checked = check_condition_var(table, att_name, filter, sign, i, ptr);
        for(int k = 0; k < table->att_num; k++) {

            if(!condition_checked) {
                incr_pointer(table, k, ptr, &ptr);
            } else {
                
                if(select_contains(input, table->attributes[k], length)) {
                    if(strncmp(table->attribute_type[k], "char", 4) == 0) {
                        printf("%s ", (char*)ptr);
                        ptr += table->att_sizes[k];
                    } else if(strcmp(table->attribute_type[k], "smallint") == 0 || strcmp(table->attribute_type[k], "integer") == 0) {
                        printf("%d ", *((int*)ptr));
                        ptr += table->att_sizes[k];
                    } else if(strncmp(table->attribute_type[k], "varchar", 7) == 0) {
                        int len = *((int*)ptr);
                        ptr += sizeof(int);
                        char* str = (char*)ptr;
                        printf("%s ", str);
                        ptr += len;
                    }
                } else {
                    incr_pointer(table, k, ptr, &ptr);
                }
            }
        }

        if(condition_checked) printf("\n");
    }
}

/**
 * Executes the select statement
 */
void execute_select(Table* table, char* att_name, int filter, char* sign, char** input, int length) {
    
    for(int i = 0; i < table->num_rows; i++) {
        void* temp = find_attribute_loc(table, att_name);
        temp = temp + i * table->row_size;

        if(is_row(table, att_name, filter, sign, i, temp)) {
            void* loc = table->root + i * table->row_size;

            for(int k = 0; k < length; k++) {
                int offset = 0;
                char* type = find_data_type(table, input[k], &offset);
                
                if(strncmp(type, "char", 4) == 0) {
                    printf("%s ", (char*)(loc + offset));
                } else if(strcmp(type, "smallint") == 0 || strcmp(type, "integer") == 0) {
                    printf("%d ", *((int*)(loc + offset)));
                }
            }
            printf("\n");
        }
    }
}

/**
 * Reads in the select statement and calls corresponding methods
 */
void select(const char *table_name, int length, ...) {
    int len = 0;
    char** to_print_atts;

    va_list arg_list;
    va_start(arg_list, length);

    Table* table = find_table(table_name);
    if(table == NULL) {
        printf("The table named %s does not exist! \n", table_name);
        return;
    }

    char* temp = va_arg(arg_list, char*);

    if(strcmp(temp, "*") == 0) {
        len = table->att_num;
        to_print_atts = (char**) malloc (table->att_num * sizeof (char*));
        char** temp = to_print_atts;
        int m = 0;
        while(m < table->att_num) {
            *temp = (char*) malloc (strlen(table->attributes[m]) * sizeof(char));
            *temp = table->attributes[m];
            temp++;
            m++;
        }
    } else {
        to_print_atts = prepare_select_statement(temp, &len);
    }
    
    printf ("\nSELECT %s\n", temp);
    printf (" FROM %s\n", table_name);

    if (length == 1) {
        execute_select(table, "", 0, "", to_print_atts, len);
    }
    else if(length == 2) {
        char* condition = va_arg(arg_list, char*);
        char* end = strdup(condition);
        char* att = strsep(&end, " ");
        char* symbol = strsep(&end, " ");
        int value = atoi(strsep(&end, " "));
        
        printf ("WHERE %s;\n", condition);

        if(table->containsVar) {            
            select_with_varchar(table, att, value, symbol, to_print_atts, len);
        } else {
            execute_select(table, att, value, symbol, to_print_atts, len);
        }
        
    }

    free(to_print_atts);
}

void free_all() {
    DP_primary* dp_curr = dp_prim_root;

    while(dp_curr != NULL) {
        Table* curr_table = dp_curr->head;

        while(curr_table != NULL) {            

            for(int i = 0; i < curr_table->att_num; i++) {     
                free(curr_table->attributes[i]);
                free(curr_table->attribute_type[i]);
            }

            free(curr_table->att_sizes);            
            free(curr_table->attributes);
            free(curr_table->attribute_type);
            free(curr_table->primary_keys);
            free(curr_table->root);

            Table* temp = curr_table->next; 
            free(curr_table);
            curr_table = temp;
        }

        DP_primary* temp = dp_curr->next;
        free(dp_curr); 
        dp_curr = temp;
    }

    printf("All memory freed!\n");
}

int main() {
    dp_prim_root = malloc(sizeof(DP_primary));
    dp_prim_root->head = NULL;
    dp_prim_root->tail = NULL;
    dp_prim = dp_prim_root;

    //Testing the insertion for all variables
    Table* table_movies = create_table("movies", "id", 8,
                "id", "smallint",
                "title", "char(30)",
                "studio", "varchar(40)",
                "length", "integer");
    
    insert("movies", 4, 1, "Three Idiots", "Paramount", 20);
    insert("movies", 4, 2, "Big Bang Theory", "Walt Disney Studios", 300);
    insert("movies", 4, 3, "Lord of the Rings", "Universal Pictures", 50);
    insert("movies", 4, 3, "Harry Potter", "Universal Pictures", 50);  //should not be added - works
    insert("movies", 4, 4, "About Time", "Warner Bros", 30);
    print_table(find_table("movies"));

    //Testing the select functionality for all variables
    select("movies", 2, "title, length", "length < 40");
    select("movies", 2, "*", "length > 10");
    select("movies", 2, "id, title, studio", "id > 2");
    select("movies", 2, "id, title", "length = 100");  //edge case: should be empty - works
    select("movies", 2, "title, studio, length", "length != 30");

    //Testing the update functioinalities 
    update("movies", 3, "length", 80, "id = 1");          //updating integer
    update("movies", 3, "id", 5, "id = 4");                //updating smallint
    update("movies", 3, "title", "Bridgerton", "id = 3");  //updating char(n)
    update("movies", 3, "studio", "20th Century", "length = 50");   //updating varchar(n)

    //updating the same variable multiple times - works
    update("movies", 3, "title", "Friends", "id = 3");
    update("movies", 3, "title", "Euphoria", "id = 3");

    //updating the primary key with pre-existing value
    update("movies", 3, "id", 2, "id = 3");   //shouldn't be added - works


    //Testing of multiples tables
    create_table("classes", "id", 6,
                "id", "smallint",
                "year", "integer",
                "name", "char(30)" );

    insert("classes", 3, 1, 2020, "Intro to CS");
    insert("classes", 3, 2, 2021, "Data structures and algorithms");
    insert("classes", 3, 3, 2010, "Artifical Intelligence");
    print_table(find_table("classes"));

    select("classes", 2, "name, year", "year > 2020");
    select("classes", 2, "*", "year < 2019");
    select("classes", 2, "*", "id < 3");

    update("classes", 3, "year", 2022, "year = 2021");
    update("classes", 3, "name", "Machine Learning", "id = 3");


    create_table("dorms", "id", 6,
                "id", "smallint",
                "name", "char(40)",
                "capacity", "integer" );
    
    insert("dorms", 3, 1, "Watson Courts", 50);
    insert("dorms", 3, 2, "South College", 150);
    insert("dorms", 3, 3, "Ruef Hall", 200);
    print_table(find_table("dorms"));

    select("dorms", 2, "name, capacity", "capacity > 100");
    select("dorms", 2, "*", "id < 3");

    update("dorms", 3, "capacity", 180, "id = 3");
    update("dorms", 3, "name", "Keefe Hall", "capacity = 150");

    free_all();
}