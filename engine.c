#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

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
    void* root;
    void* cursor;
    struct Table* next; 
} Table;

typedef struct {
    Table* head;                //A list of Tables????
    Table* tail;
    struct DP_primary* next;
} DP_primary;

DP_primary * dp_prim;

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
 * Creates a new table and stores 
 */
void create_table(const char *table_name, const char *key, int length, ...) {
    
    va_list arg_list;
    va_start(arg_list, length); 

    Table* table = malloc(sizeof(Table));               //Probably needs to be pointed by a dp_primary
    table->name = table_name;
    table->next = NULL;
    table->root = malloc(256 * sizeof(void));
    table->cursor = table->root;
    table->attribute_type = malloc(length/2 * (sizeof(char*)));
    table->attributes = malloc(length/2 * sizeof(char*));
    table->att_sizes = malloc(length/2 * sizeof(int));
    table->primary_key = key;
    table->att_num = length/2;
    table->num_rows = 0;
    table->row_size = 0;
    table->containsVar = false;

    //if the current dp_prim is full, create the next one.

    if(dp_prim->head == NULL) {
        dp_prim->head = table;
        dp_prim->tail = table;
    } else {
        dp_prim->tail->next = table;
        dp_prim->tail = table; 
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
        table->row_size += find_size(temp);                 //Defining the row_size here, but would be variable with varchar and real
        printf(" %s,\n", temp);

        if(strncmp(temp, "varchar", 7) == 0) {
            table->containsVar = true;
        }
    }

    printf (" PRIMARY KEY (%s)\n", key);
    printf (");\n");
    va_end(arg_list);
}

/**
 * Given a table name, it finds the table with the specified name,
 * returns a pointer to that table.
 * If a table with a given name doesn't exits, NULL pointer is returned. 
 */
Table* find_table (char* table_name) {
    Table* curr = dp_prim->head;

    while (true) {
        if(strcmp(curr->name, table_name) == 0) {
            return curr;
        }

        curr = curr->next;
    }
    return NULL;
}

// void serialize_row(Row* source, void* destination) {
//   memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
//   memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
//   memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
// }

// void deserialize_row(void* source, Row* destination) {
//   memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
//   memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
//   memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
// }

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
        else if (strncmp(table->attribute_type[i], "varchar", 7) == 0) {        //Not printing properly - could be insertion issue
            int len = *((int*)ptr);
            ptr += sizeof(int*);
            // char* str = (char*)ptr;
            char* str;
            // strncpy(str, (char*)ptr, len);
            // printf("%s ", str);
            printf("%s ", (char*)ptr);
            ptr += len * sizeof(char);

            // char* val = va_arg(arg_list, char*);
            // int len = strlen(val);
            // *((int *)temp) = len;
            // temp += sizeof(int*);
            // strcpy((char*)temp, val);
            // printf("’%s’", (char*)temp);
            // temp += len * sizeof(char);
        }
    }
    *temp = ptr;
    printf("\n");
}

void print_table(Table* table) {
    void *ptr = table->root;
    void** temp = &ptr;
    // printf("Value at root in printtable: %d\n", *((int*)(table->root)) );
    printf("Printing table:\n");
    int row_cnt = 1;
    
    while(row_cnt <= table->num_rows) {
        printf("Row num: %d\n", row_cnt);
        print_row(table, ptr, temp);      
        ptr = *temp;
        row_cnt++;
    }
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

    printf ("INSERT INTO %s VALUES (", table_name);
    void *temp = table->cursor;
        
    for(int i = 0; i < length; i++) {
        if(strcmp(table->attribute_type[i], "smallint") == 0) {

            int val = va_arg(arg_list, int);
            *((int *)temp) = val;
            // temp += sizeof(int*);
            temp += 8;
            table->att_sizes[i] = 8;
            // printf("PTR: %d", *((int *)temp));
            // printf("%d", val);
            printf("%d", *((int *)temp));                        

        } else if(strcmp(table->attribute_type[i], "integer") == 0) {

            int val = va_arg(arg_list, int);
            *((int *)temp) = val;
            temp += sizeof(int*);
            table->att_sizes[i] = 16;
            printf("%d", val);                               

        } else if (strncmp(table->attribute_type[i], "char", 4) == 0) {
        
            char* val = va_arg(arg_list, char*);
            strncpy((char*)temp, val, strlen(val));
            int size = find_size(table->attribute_type[i]);
            temp += size * sizeof(char);
            table->att_sizes[i] = size;
            printf("’%s’", val);

        } else if (strncmp(table->attribute_type[i], "varchar", 7) == 0) { 
            
            char* val = va_arg(arg_list, char*);
            int len = strlen(val);
            *((int *)temp) = len;
            temp += sizeof(int*);
            strcpy((char*)temp, val);
            printf("’%s’", (char*)temp);
            temp += len * sizeof(char);
        }

        if(i < length-1) {
            printf(", ");
        }
    }
    printf (");\n");

    //printing the inserted values                      //Could make a separate method
    // void *ptr = table->cursor;
    // for(int i = 0; i < length; i++) {
    //     if(strcmp(table->attribute_type[i], "smallint") == 0) {
    //         printf("%d\n", *((int*)ptr));
    //         ptr += find_size(table->attribute_type[i]);
    //     } 
    //     else if(strcmp(table->attribute_type[i], "integer") == 0) {
    //         printf("%d\n", *((int*)ptr));
    //         ptr += find_size(table->attribute_type[i]);
    //     } 
    //     else if (strncmp(table->attribute_type[i], "char", 4) == 0) {
    //         char* str = (char*)ptr;
    //         printf("%s\n", str);
    //         ptr += find_size(table->attribute_type[i]) * sizeof(char);
    //     }
    //     else if (strncmp(table->attribute_type[i], "varchar", 7) == 0) {
    //         int len = *((int*)ptr);
    //         ptr += sizeof(int*);
    //         char* str = (char*)ptr;
    //         printf("%s\n", str);
    //         ptr += len;
    //     }
    // }

    print_row(table, table->cursor, &(table->cursor));
    
    table->cursor = temp;
    table->num_rows++;
    // print_table(table);
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
 */                                                                                                 //delete num_row
bool is_row(Table* table, char* att_name, int value, char* sign, int row_num, void* temp) {         //Works only when the condition is comparing ints

    
    if(strcmp(sign, "=") == 0 && *((int*)temp) == value) {
        return true;
    }
    else if(strcmp(sign, "!=") == 0 && *((int*)temp) != value) return true;
    else if(strcmp(sign, ">") == 0 && *((int*)temp) > value) {
        return true;
    }
    else if(strcmp(sign, "<") == 0 && *((int*)temp) < value) return true;
    else if(strcmp(sign, " ") == 0) return true;

    return false;
}

bool check_condition_var(Table* table, char* att_name, int filter, char* sign, int num_row, void* ptr) {
    void* temp = ptr;

    // printf("VALUE: %d", *((int*)temp));
    for(int i = 0; i < table->att_num; i++) {
        if(strcmp(att_name, table->attributes[i]) == 0 && is_row(table, att_name, filter, sign, num_row, temp)) {
            // *ptr = temp;
            // printf("Cooooommeeee");
            return true;
        }
        
        if(strncmp(table->attribute_type[i], "varchar", 7) == 0) {
            int len = *((int*)temp);
            temp += sizeof(int*);
            temp += len;
        } 
        else {
            temp += table->att_sizes[i];
        }
    }

    return false;
}

void update_with_varchar(Table* table, char* att_name, int filter, char* sign,  int new_int, char* new_str, char* to_update) {
    void* ptr = table->root;
    int cntr = 0;

    for(int i = 0; i < table->num_rows; i++) {
        bool condition_checked = check_condition_var(table, att_name, filter, sign, i, ptr);

        for(int k = 0; k < table->att_num; k++) {
            if(strcmp(table->attributes[k], to_update) == 0 && condition_checked) {
                // printf("before: %d, %s, %d\n", *((int*)ptr), (char*)(ptr + 8), *((int*)(ptr + 38)));

                if (new_int != NULL) {
                    *((int*)(ptr)) = new_int;                      
                    ptr += find_size(table->attribute_type[k]);
                } 
                else if (new_str != NULL && strncmp(table->attribute_type[k], "char", 4) == 0) {
                    strcpy((char*)(ptr), new_str);
                    ptr += find_size(table->attribute_type[k]);
                }
                else if (new_str != NULL && strncmp(table->attribute_type[k], "varchar", 7) == 0) {         //What if the new value is longer than the initial
                    int len = *((int*)ptr);
                    ptr += sizeof(int*);
                    char* str = (char*)ptr;
                    // printf("BEEEEfore: %s\n", str);
                    strcpy((char*)(ptr), new_str);
                    // printf("AAAAFter: %s\n", (char*)(ptr));
                    ptr += len;
                }

                // printf("after: %d, %s, %d\n", *((int*)loc), (char*)(loc + 8), *((int*)(loc + 38)));
            
            } else {
                if(strncmp(table->attribute_type[k], "varchar", 7) == 0) {
                    int len = *((int*)ptr);
                    ptr += sizeof(int*);
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
 * 
 */
void execute_update(Table* table, char* att_name, int filter, char* sign, int offset,  int new_int, char* new_str) {
    for(int i = 0; i < table->num_rows; i++) {
        void* temp = find_attribute_loc(table, att_name);
        // printf("BEFORE add: %d\n", *((int*)temp));
        temp = temp + (i * (table->row_size));
        // printf("After add: %d\n", *((int*)temp));

        if(is_row(table, att_name, filter, sign, i, temp) == true) {
            // printf("HERE at %d\n", i);
            void* loc = table->root + i * table->row_size;
            printf("before: %d, %s, %d\n", *((int*)loc), (char*)(loc + 8), *((int*)(loc + 38)));

            if (new_int != NULL) {
                *((int*)(loc + offset)) = new_int;                     
            } else if (new_str != NULL) {
                strcpy((char*)(loc+offset), new_str);
            }

            printf("after: %d, %s, %d\n", *((int*)loc), (char*)(loc + 8), *((int*)(loc + 38)));
        }
    }
}

/**
 * 
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
    printf ("UPDATE %s SET ", table_name);
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

        temp += table->att_sizes[i];                //How to add the sizes for varchars?????????
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

bool select_contains(char** list, char* type, int length) {
    for(int i = 0; i < length; i++) {
        if(strcmp(list[i], type) == 0) {
            return true;
        }
    }

    return false;
}

void incr_pointer(Table* table, int k, void* ptr, void** ret) {
    if(strncmp(table->attribute_type[k], "char", 4) == 0) {
        ptr += table->att_sizes[k];
        
    } else if(strcmp(table->attribute_type[k], "smallint") == 0 || strcmp(table->attribute_type[k], "integer") == 0) {
        ptr += 8;       //different for integer!
        
    } else if(strncmp(table->attribute_type[k], "varchar", 7) == 0) {
        int len = *((int*)ptr);
        ptr += sizeof(int*);
        ptr += len;
    }

    *ret = ptr;
}

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
                        ptr += sizeof(int*);
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
 * Prints the values of the selected attributes.
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
        to_print_atts = table->attributes;
        len = table->att_num;
    } else {
        to_print_atts = prepare_select_statement(temp, &len);
    }
    
    printf ("SELECT %s\n", temp);
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

    free(to_print_atts);            //Need to free the inner mallocs
}

int main() {
    dp_prim = malloc(sizeof(DP_primary));
    dp_prim->head = NULL;
    dp_prim->tail = NULL;

    // create_table("movies", "id", 6,
    //             "id", "smallint",
    //             "title", "char(30)",
    //             "length", "smallint" );
    
    // insert("movies", 3, 1, "Lyle, Lyle, Crocodile", 100);
    // insert("movies", 3, 2, "Big Bang Theory", 50);
    
    // create_table("classes", "id", 6,
    //             "id", "smallint",
    //             "year", "smallint",
    //             "name", "char(30)" );

    // insert("classes", 3, 1, 2020, "Intro to CS");
    // insert("classes", 3, 2, 2021, "Data structures and algorithms");

    // // insert("movies", 3, 3, "Harry Porter", 70);

    // // update("movies", 3, "length", 150, "id = 1");
    // // update("movies", 3, "title", "Friends", "id = 2");

    // // select("movies", 2, "title, length", "length < 200");
    // // select("movies", 2, "*", "length > 60");
    // select("classes", 2, "id, name", "year > 2010");
    // select("classes", 2, "*", "year < 2022");

    create_table("movies", "id", 6,
                "id", "smallint",
                "title", "varchar(30)",
                "length", "smallint" );

    insert("movies", 3, 1, "Cuckoo's nest", 100);
    insert("movies", 3, 2, "Big Bang Theory", 50);
    insert("movies", 3, 3, "Loook both ways", 200);
    insert("movies", 3, 4, "Eternal Sunshine", 70);
    update("movies", 3, "title", "Friends", "id = 2");
    update("movies", 3, "length", 20, "id = 2");
    // select("movies", 2, "title, length", "length < 80");
    // select("movies", 2, "title, length", "length != 100");
    // select("movies", 2, "*", "length > 90");
}