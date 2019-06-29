/*
** Created by doom on 29/06/19.
*/

#include <gtest/gtest.h>
#include <dagger.hpp>

TEST(dagger, construction)
{
    const char *words[] = {
        "abaca",
        "abacas",
        "abacost",
        "abacosts",
        "abacule",
        "abacules",
        "abaissa",
        "abaissable",
        "balader",
    };
    auto dawg = doom::dagger::from_dictionary(std::begin(words), std::end(words));

    for (auto &&word : words) {
        ASSERT_TRUE(dawg.contains(word));
    }
    ASSERT_FALSE(dawg.contains("balade"));
}
