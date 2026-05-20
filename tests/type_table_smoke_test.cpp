#include <cassert>
#include <iostream>

#include "type_table.hpp"

int main() {
    TypeTable table;

    assert(table.has_type("Object"));
    assert(table.has_type("Number"));
    assert(table.has_type("String"));
    assert(table.has_type("Boolean"));

    table.register_type(TypeInfo{"Foo", "Object"});
    table.register_type(TypeInfo{"Bar", "Foo"});

    assert(table.conforms_to("Bar", "Foo"));
    assert(table.conforms_to("Bar", "Object"));
    assert(!table.conforms_to("Foo", "Bar"));

    assert(table.lowest_common_ancestor("Foo", "Bar") == "Foo");

    table.ensure_vector_type("Number");
    table.ensure_vector_type("Object");

    const auto vec_number = TypeTable::make_vector_type("Number");
    const auto vec_object = TypeTable::make_vector_type("Object");
    assert(table.conforms_to(vec_number, vec_object));

    table.ensure_iterable_type("Number");
    table.ensure_iterable_type("Object");

    const auto iter_number = TypeTable::make_iterable_type("Number");
    const auto iter_object = TypeTable::make_iterable_type("Object");
    assert(table.lowest_common_ancestor(iter_number, iter_object) == iter_object);

    const auto& vec_info = table.get_type(vec_number);
    assert(vec_info.methods.count("size") == 1);
    assert(vec_info.methods.count("iter") == 1);
    assert(vec_info.methods.at("iter").return_type == iter_number);

    const auto& iter_info = table.get_type(iter_number);
    assert(iter_info.methods.count("next") == 1);
    assert(iter_info.methods.count("current") == 1);
    assert(iter_info.methods.at("current").return_type == "Number");

    // Vector<T> conforma a Iterable<T>
    assert(table.conforms_to(vec_number, iter_number));
    assert(table.conforms_to(vec_object, iter_object));
    // La covarianza funciona: Vector<Number> conforma a Iterable<Object>
    assert(table.conforms_to(vec_number, iter_object));
    // Pero no al revés
    assert(!table.conforms_to(iter_number, vec_number));

    // LCA mixto (contenedor vs tipo regular) debe ser Object
    assert(table.lowest_common_ancestor(vec_number, "String") == "Object");
    assert(table.lowest_common_ancestor(iter_number, "Number") == "Object");
    assert(table.lowest_common_ancestor(vec_number, iter_number) == "Object");

    bool caught = false;
    try {
        table.register_type(TypeInfo{"Bad", "Number"});
    } catch (const std::exception&) {
        caught = true;
    }
    assert(caught && "Herencia desde Number debe fallar");

    std::cout << "TypeTable smoke test passed." << std::endl;
    return 0;
}
