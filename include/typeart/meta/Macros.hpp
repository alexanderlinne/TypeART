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

#define META_DECL_REF_ANY(R, DATA, I, ELEM) BOOST_PP_CAT(META_CALL_DECL_MACRO_, BOOST_PP_TUPLE_SIZE(ELEM))(ELEM)

#define META_DECL_MEMBER_IMPL(TYPE, NAME, GETTER_SPECIFIERS) \
 private:                                                    \
  TYPE NAME;                                                 \
                                                             \
 public:                                                     \
  TYPE META_CAT2(get_, NAME)() const GETTER_SPECIFIERS;      \
  void META_CAT2(set_, NAME)(TYPE new_value);

#define META_CALL_MEMBER_DECL_MACRO_2(ELEM) \
  META_DECL_MEMBER_IMPL(BOOST_PP_TUPLE_ELEM(0, ELEM), BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_EMPTY())

#define META_CALL_MEMBER_DECL_MACRO_3(ELEM) \
  META_DECL_MEMBER_IMPL(BOOST_PP_TUPLE_ELEM(0, ELEM), BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_DECL_MEMBER(R, DATA, I, ELEM) BOOST_PP_CAT(META_CALL_MEMBER_DECL_MACRO_, BOOST_PP_TUPLE_SIZE(ELEM))(ELEM)

#define META_CLASS(BASE, KIND, REFS, MEMBERS)                                                                    \
 public:                                                                                                         \
  inline KIND() : BASE(meta_id_t::invalid, Kind::KIND, std::vector<Meta*>(BOOST_PP_ARRAY_SIZE(REFS), nullptr)) { \
  }                                                                                                              \
                                                                                                                 \
 protected:                                                                                                      \
  virtual bool compare(const Meta& other) const override;                                                        \
                                                                                                                 \
 public:                                                                                                         \
  constexpr static Kind META_KIND = Kind::KIND;                                                                  \
  static inline bool classof(Kind kind) {                                                                        \
    switch (kind) {                                                                                              \
      case Kind::KIND:                                                                                           \
        return true;                                                                                             \
      default:                                                                                                   \
        return false;                                                                                            \
    }                                                                                                            \
  }                                                                                                              \
  virtual ~KIND();                                                                                               \
                                                                                                                 \
  const char* get_ref_name(size_t idx) const override;                                                           \
                                                                                                                 \
  size_t get_member_count() const override;                                                                      \
  const char* get_member_name(size_t idx) const override;                                                        \
  std::string serialize_member(size_t idx) const override;                                                       \
  void deserialize_member(size_t idx, const std::string& value) override;                                        \
                                                                                                                 \
  BOOST_PP_LIST_FOR_EACH_I(META_DECL_REF_ANY, _, BOOST_PP_ARRAY_TO_LIST(REFS))                                   \
  BOOST_PP_LIST_FOR_EACH_I(META_DECL_MEMBER, _, BOOST_PP_ARRAY_TO_LIST(MEMBERS))

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

#define META_CALL_IMPL_MACRO_4(CLASS, I, ELEM)  \
  META_IMPL_MACRO(BOOST_PP_TUPLE_ELEM(0, ELEM)) \
  (CLASS, I, BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_IMPL_REF_ANY(R, DATA, I, ELEM) \
  BOOST_PP_CAT(META_CALL_IMPL_MACRO_, BOOST_PP_TUPLE_SIZE(ELEM))(DATA, I, ELEM)

#define META_IMPL_MEMBER_IMPL(CLASS, TYPE, NAME, GETTER_SPECIFIERS) \
  TYPE CLASS::META_CAT2(get_, NAME)() const {                       \
    return NAME;                                                    \
  }                                                                 \
  void CLASS::META_CAT2(set_, NAME)(TYPE new_value) {               \
    NAME = new_value;                                               \
  }

#define META_CALL_MEMBER_IMPL_MACRO_2(CLASS, ELEM) \
  META_IMPL_MEMBER_IMPL(CLASS, BOOST_PP_TUPLE_ELEM(0, ELEM), BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_EMPTY())

#define META_CALL_MEMBER_IMPL_MACRO_3(CLASS, ELEM) \
  META_IMPL_MEMBER_IMPL(CLASS, BOOST_PP_TUPLE_ELEM(0, ELEM), BOOST_PP_TUPLE_ELEM(1, ELEM), BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_IMPL_MEMBER(R, DATA, I, ELEM) \
  BOOST_PP_CAT(META_CALL_MEMBER_IMPL_MACRO_, BOOST_PP_TUPLE_SIZE(ELEM))(DATA, ELEM)

#define META_IMPL_MEMBER_COMPARE(R, DATA, ELEM) \
  &&META_CAT2(get_, BOOST_PP_TUPLE_ELEM(1, ELEM))() == other_casted->META_CAT2(get_, BOOST_PP_TUPLE_ELEM(1, ELEM))()

#define META_IMPL_REF_NAME_STRING(ELEM) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1, ELEM))

#define META_IMPL_REF_NAME_REF(ELEM) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_IMPL_REF_NAME_TUPLE(ELEM) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_IMPL_REF_NAME_OPTIONAL(ELEM) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, ELEM))

#define META_IMPL_REF_NAME_OVERLOAD(ELEM) BOOST_PP_CAT(META_IMPL_REF_NAME_, BOOST_PP_TUPLE_ELEM(0, ELEM))(ELEM)

#define META_IMPL_REF_NAME(R, DATA, I, ELEM) \
  case I:                                    \
    return META_IMPL_REF_NAME_OVERLOAD(ELEM);

#define META_IMPL_MEMBER_NAME(R, DATA, I, ELEM) \
  case I:                                       \
    return BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(1, ELEM));

#define META_IMPL_MEMBER_SERIALIZE(R, DATA, I, ELEM) \
  case I:                                            \
    return ::meta::serialize<BOOST_PP_TUPLE_ELEM(0, ELEM)>(META_CAT2(get_, BOOST_PP_TUPLE_ELEM(1, ELEM))());

#define META_IMPL_MEMBER_DESERIALIZE(R, DATA, I, ELEM)                                                       \
  case I:                                                                                                    \
    META_CAT2(set_, BOOST_PP_TUPLE_ELEM(1, ELEM))(::meta::deserialize<BOOST_PP_TUPLE_ELEM(0, ELEM)>(value)); \
    break;

#define META_CLASS_IMPL(BASE, KIND, REFS, MEMBERS)                                                                 \
  bool KIND::compare(const Meta& other) const {                                                                    \
    if (other.get_kind() != get_kind()) {                                                                          \
      return false;                                                                                                \
    }                                                                                                              \
    auto other_casted = meta::dyn_cast<KIND>(&other);                                                              \
    other_casted;                                                                                                  \
    return get_refs() == other.get_refs()                                                                          \
                             BOOST_PP_LIST_FOR_EACH(META_IMPL_MEMBER_COMPARE, _, BOOST_PP_ARRAY_TO_LIST(MEMBERS)); \
  }                                                                                                                \
                                                                                                                   \
  KIND::~KIND() {                                                                                                  \
  }                                                                                                                \
                                                                                                                   \
  const char* KIND::get_ref_name(size_t idx) const {                                                               \
    switch (idx) {                                                                                                 \
      BOOST_PP_LIST_FOR_EACH_I(META_IMPL_REF_NAME, KIND, BOOST_PP_ARRAY_TO_LIST(REFS))                             \
      default:                                                                                                     \
        abort();                                                                                                   \
    }                                                                                                              \
  }                                                                                                                \
                                                                                                                   \
  size_t KIND::get_member_count() const {                                                                          \
    return BOOST_PP_ARRAY_SIZE(MEMBERS);                                                                           \
  }                                                                                                                \
                                                                                                                   \
  const char* KIND::get_member_name(size_t idx) const {                                                            \
    switch (idx) {                                                                                                 \
      BOOST_PP_LIST_FOR_EACH_I(META_IMPL_MEMBER_NAME, KIND, BOOST_PP_ARRAY_TO_LIST(MEMBERS))                       \
      default:                                                                                                     \
        abort();                                                                                                   \
    }                                                                                                              \
  }                                                                                                                \
                                                                                                                   \
  std::string KIND::serialize_member(size_t idx) const {                                                           \
    switch (idx) {                                                                                                 \
      BOOST_PP_LIST_FOR_EACH_I(META_IMPL_MEMBER_SERIALIZE, KIND, BOOST_PP_ARRAY_TO_LIST(MEMBERS))                  \
      default:                                                                                                     \
        abort();                                                                                                   \
    }                                                                                                              \
  }                                                                                                                \
                                                                                                                   \
  void KIND::deserialize_member(size_t idx, const std::string& value) {                                            \
    switch (idx) {                                                                                                 \
      BOOST_PP_LIST_FOR_EACH_I(META_IMPL_MEMBER_DESERIALIZE, KIND, BOOST_PP_ARRAY_TO_LIST(MEMBERS))                \
      default:                                                                                                     \
        abort();                                                                                                   \
    }                                                                                                              \
  }                                                                                                                \
                                                                                                                   \
  BOOST_PP_LIST_FOR_EACH_I(META_IMPL_REF_ANY, KIND, BOOST_PP_ARRAY_TO_LIST(REFS))                                  \
  BOOST_PP_LIST_FOR_EACH_I(META_IMPL_MEMBER, KIND, BOOST_PP_ARRAY_TO_LIST(MEMBERS))
