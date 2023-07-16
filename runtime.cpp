#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <utility>

using namespace std;

namespace runtime {

// -------------------------------- ObjectHolder -------------------------------- //
    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
            : data_(std::move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const {
        return data_.get();
    }

    ObjectHolder::operator bool() const {
        return Get() != nullptr;
    }

// -------------------------------- Bool -------------------------------- //
    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }

// -------------------------------- Class -------------------------------- //
    Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
            : parent_(parent)
            , name_(std::move(name))
            , methods_(std::move(methods)) {
        if (parent != nullptr) {
            for (const auto& method : parent->methods_) {
                name_to_method_[method.name] = &method;
            }
        }
        for (const auto& method : methods_) {
            name_to_method_[method.name] = &method;
        }
    }

    const Method* Class::GetMethod(const std::string& name) const {
        return name_to_method_.count(name) ? name_to_method_.at(name) : nullptr;
    }

    const std::string& Class::GetName() const {
        return this->name_;
    }

    void Class::Print(ostream& os, Context& /* context */) {
        os << "Class "sv;
        os << GetName();
    }

// -------------------------------- ClassInstance -------------------------------- //
    ClassInstance::ClassInstance(const Class& cls)
            : cls_(cls) {
    }

    bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
        auto method_ptr = cls_.GetMethod(method);

        return method_ptr && method_ptr->formal_params.size() == argument_count;
    }

    ObjectHolder ClassInstance::Call(const std::string& method,
                                     const std::vector<ObjectHolder>& actual_args,
                                     Context& context) {
        if (!HasMethod(method, actual_args.size())) {
            throw std::runtime_error("Not implemented"s);
        }

        auto* method_ptr = cls_.GetMethod(method);
        Closure closure;

        closure["self"s] = ObjectHolder::Share(*this);

        for (size_t i = 0; i < actual_args.size(); ++i) {
            closure[method_ptr->formal_params[i]] = actual_args[i];
        }

        return method_ptr->body->Execute(closure, context);
    }

    void ClassInstance::Print(std::ostream& os, Context& context) {
        auto method_ptr = this->cls_.GetMethod("__str__"s);

        if (method_ptr != nullptr) {
            Call(method_ptr->name, {}, context)->Print(os, context);
        } else {
            os << this;
        }
    }

    Closure& ClassInstance::Fields() {
        return fields_;
    }

    const Closure& ClassInstance::Fields() const {
        return fields_;
    }

// -------------------------------- Function -------------------------------- //
    bool IsTrue(const ObjectHolder& object) {
        auto ptr_b = object.TryAs<runtime::Bool>();
        if (ptr_b != nullptr) {
            return ptr_b->GetValue();
        }

        auto ptr_n = object.TryAs<runtime::Number>();
        if (ptr_n != nullptr) {
            return ptr_n->GetValue() != 0;
        }

        auto ptr_s = object.TryAs<runtime::String>();
        if (ptr_s != nullptr) {
            return ptr_s->GetValue() != ""s;
        }

        return false;
    }
/*
 * Возвращает true, если lhs и rhs содержат одинаковые числа, строки или значения типа Bool.
 * Если lhs - объект с методом __eq__, функция возвращает результат вызова lhs.__eq__(rhs),
 * приведённый к типу Bool. Если lhs и rhs имеют значение None, функция возвращает true.
 * В остальных случаях функция выбрасывает исключение runtime_error.
 *
 * Параметр context задаёт контекст для выполнения метода __eq__
 */
    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs && !rhs) {
            return true;
        }

        if (!lhs) {
            throw std::runtime_error("Cannot compare objects for equality"s);
        }

        auto l_ptr_n = lhs.TryAs<runtime::Number>();
        auto r_ptr_n = rhs.TryAs<runtime::Number>();
        if (l_ptr_n != nullptr && r_ptr_n != nullptr) {
            return l_ptr_n->GetValue() == r_ptr_n->GetValue();
        }

        auto l_ptr_s = lhs.TryAs<runtime::String>();
        auto r_ptr_s = rhs.TryAs<runtime::String>();
        if (l_ptr_s != nullptr && r_ptr_s != nullptr) {
            return l_ptr_s->GetValue() == r_ptr_s->GetValue();
        }

        auto l_ptr_b = lhs.TryAs<runtime::Bool>();
        auto r_ptr_b = rhs.TryAs<runtime::Bool>();
        if (l_ptr_b != nullptr && r_ptr_b != nullptr) {
            return l_ptr_b->GetValue() == r_ptr_b->GetValue();
        }

        auto l_ptr_class_inst = lhs.TryAs<runtime::ClassInstance>();
        constexpr int EQ_METHOD_ARGS_COUNT = 1;
        if (l_ptr_class_inst != nullptr && l_ptr_class_inst->HasMethod("__eq__"s, EQ_METHOD_ARGS_COUNT)) {
            auto res = l_ptr_class_inst->Call("__eq__"s, { rhs }, context);
            return res.TryAs<Bool>()->GetValue();
        }

        throw std::runtime_error("Cannot compare objects for equality"s);
    }
/*
 * Если lhs и rhs - числа, строки или значения bool, функция возвращает результат их сравнения
 * оператором <.
 * Если lhs - объект с методом __lt__, возвращает результат вызова lhs.__lt__(rhs),
 * приведённый к типу bool. В остальных случаях функция выбрасывает исключение runtime_error.
 *
 * Параметр context задаёт контекст для выполнения метода __lt__
 */
    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs) {
            throw std::runtime_error("Cannot compare objects for equality"s);
        }

        auto l_ptr_n = lhs.TryAs<runtime::Number>();
        auto r_ptr_n = rhs.TryAs<runtime::Number>();
        if (l_ptr_n != nullptr && r_ptr_n != nullptr) {
            return l_ptr_n->GetValue() < r_ptr_n->GetValue();
        }

        auto l_ptr_s = lhs.TryAs<runtime::String>();
        auto r_ptr_s = rhs.TryAs<runtime::String>();
        if (l_ptr_s != nullptr && r_ptr_s != nullptr) {
            return l_ptr_s->GetValue() < r_ptr_s->GetValue();
        }

        auto l_ptr_b = lhs.TryAs<runtime::Bool>();
        auto r_ptr_b = rhs.TryAs<runtime::Bool>();
        if (l_ptr_b != nullptr && r_ptr_b != nullptr) {
            return l_ptr_b->GetValue() < r_ptr_b->GetValue();
        }

        auto l_ptr_class_inst = lhs.TryAs<runtime::ClassInstance>();
        constexpr int LT_METHOD_ARGS_COUNT = 1;
        if (l_ptr_class_inst != nullptr && l_ptr_class_inst->HasMethod("__lt__"s, LT_METHOD_ARGS_COUNT)) {
            auto res = l_ptr_class_inst->Call("__lt__"s, { rhs }, context);
            return res.TryAs<Bool>()->GetValue();
        }

        throw std::runtime_error("Cannot compare objects for less"s);
    }
// Возвращает значение, противоположное Equal(lhs, rhs, context)
    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Equal(lhs, rhs, context);
    }
// Возвращает значение lhs>rhs, используя функции Equal и Less
    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
    }
// Возвращает значение lhs<=rhs, используя функции Equal и Less
    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
    }
// Возвращает значение, противоположное Less(lhs, rhs, context)
    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context);
    }
}  // namespace runtime