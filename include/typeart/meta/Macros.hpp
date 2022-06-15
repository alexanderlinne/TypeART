#pragma once

#include <boost/preprocessor.hpp>

#define META_CAT2(A, B) BOOST_PP_CAT(A, B)

#define META_CAT3(A, B, C) BOOST_PP_CAT(A, BOOST_PP_CAT(B, C))

#define META_DECL_REF(TYPE, NAME, GETTER_SPECIFIERS)           \
  const TYPE& META_CAT2(get_, NAME)() const GETTER_SPECIFIERS; \
  TYPE* META_CAT3(get_, NAME, _raw)() const;                   \
  void META_CAT3(set_, NAME, _raw)(TYPE * new_ref);

#define META_DECL_STRING(NAME, GETTER_SPECIFIERS, UNUSED)             \
 public:                                                              \
  const std::string& META_CAT2(get_, NAME)() const GETTER_SPECIFIERS; \
  String* META_CAT3(get_, NAME, _raw)() const;                        \
  void META_CAT3(set_, NAME, _raw)(String * new_ref);

#define META_DECL_INTEGER(TYPE, NAME, GETTER_SPECIFIERS) \
  TYPE META_CAT2(get_, NAME)() const GETTER_SPECIFIERS;  \
  Integer* META_CAT3(get_, NAME, _raw)() const;          \
  void META_CAT3(set_, NAME, _raw)(Integer * new_ref);

#define META_DECL_TUPLE(TYPE, NAME, GETTER_SPECIFIERS)              \
  TupleProxy<TYPE> META_CAT2(get_, NAME)() const GETTER_SPECIFIERS; \
  Tuple* META_CAT3(get_, NAME, _raw)();                             \
  void META_CAT3(set_, NAME, _raw)(Tuple * ref);

#define META_DECL_OPTIONAL(TYPE, NAME, GETTER_SPECIFIERS)              \
  OptionalProxy<TYPE> META_CAT2(get_, NAME)() const GETTER_SPECIFIERS; \
  Optional* META_CAT3(get_, NAME, _raw)();                             \
  void META_CAT3(set_, NAME, _raw)(Optional * ref);

#define META_DECL_MACRO(NAME) BOOST_PP_CAT(META_DECL_, NAME)

#define META_CALL_DECL_MACRO_2(ELEM)            \
  META_DECL_MACRO(BOOST_PP_TUPLE_ELEM(0, ELEM)) \
  (BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_EMPTY(), BOOST_PP_EMPTY())

#define META_CALL_DECL_MACRO_3(ELEM)            \
  META_DECL_MACRO(BOOST_PP_TUPLE_ELEM(0, ELEM)) \
  (BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_TUPLE_ELEM(2, ELEM), BOOST_PP_EMPTY())

#define META_CALL_DECL_MACRO_4(ELEM)            \
  META_DECL_MACRO(BOOST_PP_TUPLE_ELEM(0, ELEM)) \
  (BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_TUPLE_ELEM(2, ELEM), BOOST_PP_TUPLE_ELEM(3, ELEM))

#define META_DECL_MEMBER(R, DATA, I, ELEM) BOOST_PP_CAT(META_CALL_DECL_MACRO_, BOOST_PP_TUPLE_SIZE(ELEM))(ELEM)

#define META_DECL_COMMON(KIND)                  \
  constexpr static Kind META_KIND = Kind::KIND; \
  static inline bool classof(Kind kind) {       \
    switch (kind) {                             \
      case Kind::KIND:                          \
        return true;                            \
      default:                                  \
        return false;                           \
    }                                           \
  }                                             \
  virtual ~KIND();

#define META_CLASS_2(BASE, KIND)                                                         \
 public:                                                                                 \
  inline KIND() : BASE(meta_id_t::invalid, Kind::KIND, std::vector<Meta*>(0, nullptr)) { \
  }                                                                                      \
  META_DECL_COMMON(KIND)

#define META_CLASS_3(BASE, KIND, MEMBERS)                                                                           \
 public:                                                                                                            \
  inline KIND() : BASE(meta_id_t::invalid, Kind::KIND, std::vector<Meta*>(BOOST_PP_TUPLE_SIZE(MEMBERS), nullptr)) { \
  }                                                                                                                 \
  META_DECL_COMMON(KIND)                                                                                            \
  BOOST_PP_LIST_FOR_EACH_I(META_DECL_MEMBER, _, BOOST_PP_TUPLE_TO_LIST(MEMBERS))

#define META_CLASS(...) BOOST_PP_OVERLOAD(META_CLASS_, __VA_ARGS__)(__VA_ARGS__)

#define META_IMPL_REF(CLASS, INDEX, TYPE, NAME)             \
  const TYPE& CLASS::META_CAT2(get_, NAME)() const {        \
    return *META_CAT3(get_, NAME, _raw());                  \
  }                                                         \
  TYPE* CLASS::META_CAT3(get_, NAME, _raw)() const {        \
    return dyn_cast<TYPE>(refs[INDEX]);                     \
  }                                                         \
  void CLASS::META_CAT3(set_, NAME, _raw)(TYPE * new_ref) { \
    refs[INDEX] = new_ref;                                  \
  }

#define META_IMPL_STRING(CLASS, INDEX, NAME)                  \
  const std::string& CLASS::META_CAT2(get_, NAME)() const {   \
    return META_CAT3(get_, NAME, _raw)()->get_data();         \
  }                                                           \
  String* CLASS::META_CAT3(get_, NAME, _raw)() const {        \
    return dyn_cast<String>(refs[INDEX]);                     \
  }                                                           \
  void CLASS::META_CAT3(set_, NAME, _raw)(String * new_ref) { \
    refs[INDEX] = new_ref;                                    \
  }

#define META_IMPL_INTEGER(CLASS, INDEX, TYPE, NAME)                      \
  TYPE CLASS::META_CAT2(get_, NAME)() const {                            \
    return static_cast<TYPE>(META_CAT3(get_, NAME, _raw)()->get_data()); \
  }                                                                      \
  Integer* CLASS::META_CAT3(get_, NAME, _raw)() const {                  \
    return dyn_cast<Integer>(refs[INDEX]);                               \
  }                                                                      \
  void CLASS::META_CAT3(set_, NAME, _raw)(Integer * new_ref) {           \
    refs[INDEX] = new_ref;                                               \
  }

#define META_IMPL_TUPLE(CLASS, INDEX, TYPE, NAME)         \
  TupleProxy<TYPE> CLASS::META_CAT2(get_, NAME)() const { \
    auto result = dyn_cast<Tuple>(refs[INDEX]);           \
    assert(result != nullptr);                            \
    return {*result};                                     \
  }                                                       \
  Tuple* CLASS::META_CAT3(get_, NAME, _raw)() {           \
    return dyn_cast<Tuple>(refs[INDEX]);                  \
  }                                                       \
  void CLASS::META_CAT3(set_, NAME, _raw)(Tuple * ref) {  \
    refs[INDEX] = ref;                                    \
  }

#define META_IMPL_OPTIONAL(CLASS, INDEX, TYPE, NAME)         \
  OptionalProxy<TYPE> CLASS::META_CAT2(get_, NAME)() const { \
    auto result = dyn_cast<Optional>(refs[INDEX]);           \
    assert(result != nullptr);                               \
    return {*result};                                        \
  }                                                          \
  Optional* CLASS::META_CAT3(get_, NAME, _raw)() {           \
    return dyn_cast<Optional>(refs[INDEX]);                  \
  }                                                          \
  void CLASS::META_CAT3(set_, NAME, _raw)(Optional * ref) {  \
    refs[INDEX] = ref;                                       \
  }

#define META_IMPL_MACRO(NAME) BOOST_PP_CAT(META_IMPL_, NAME)

#define META_CALL_IMPL_MACRO_2(CLASS, I, ELEM)  \
  META_IMPL_MACRO(BOOST_PP_TUPLE_ELEM(0, ELEM)) \
  (CLASS, I, BOOST_PP_TUPLE_ELEM(1, ELEM))

#define META_CALL_IMPL_MACRO_3(CLASS, I, ELEM)  \
  META_IMPL_MACRO(BOOST_PP_TUPLE_ELEM(0, ELEM)) \
  (CLASS, I, BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_IMPL_MEMBER(R, DATA, I, ELEM) BOOST_PP_CAT(META_CALL_IMPL_MACRO_, BOOST_PP_TUPLE_SIZE(ELEM))(DATA, I, ELEM)

#define META_CLASS_IMPL_2(BASE, KIND) \
  KIND::~KIND() {                     \
  }

#define META_CLASS_IMPL_3(BASE, KIND, MEMBERS) \
  KIND::~KIND() {                              \
  }                                            \
  BOOST_PP_LIST_FOR_EACH_I(META_IMPL_MEMBER, KIND, BOOST_PP_TUPLE_TO_LIST(MEMBERS))

#define META_CLASS_IMPL(...) BOOST_PP_OVERLOAD(META_CLASS_IMPL_, __VA_ARGS__)(__VA_ARGS__)
