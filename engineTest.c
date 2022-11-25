// #include "./Unity-master/src/unity.h"
#include "engine.c"

DP_primary * dp_prim;

void test_Insert_Fixed_Variables(void)
{
    dp_prim = malloc(sizeof(DP_primary));
    dp_prim->head = NULL;
    dp_prim->tail = NULL;

    create_table("movies", "id", 6,
                "id", "smallint",
                "title", "char(30)",
                "length", "smallint" );
    
    insert("movies", 3, 1, "Lyle, Lyle, Crocodile", 100);
    // insert("movies", 3, 2, "Big Bang Theory", 50);
    // insert("movies", 3, 3, "Star Wars", 150);

    TEST_ASSERT_EQUAL_HEX8(30, find_table("movies")->cursor);

}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Insert_Fixed_Variables);
    return UNITY_END();
}