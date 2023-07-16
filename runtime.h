#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace runtime {

class Context {    // Контекст исполнения инструкций Mython
public:
    virtual std::ostream& GetOutputStream() = 0;    // Возвращает поток вывода для команд print

protected:
    ~Context() = default;
};

class Object {    // Базовый класс для всех объектов языка Mython
public:
    virtual ~Object() = default;
    virtual void Print(std::ostream& os, Context& context) = 0;    // выводит в os своё представление в виде строки
};

class ObjectHolder {    // Специальный класс-обёртка, предназначенный для хранения объекта в Mython-программе
public:
    ObjectHolder() = default;    // Создаёт пустое значение

    template <typename T>    // Возвращает ObjectHolder, владеющий объектом типа T (конкретный класс-наследник Object)
    [[nodiscard]] static ObjectHolder Own(T&& object) {
        return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
    }

    [[nodiscard]] static ObjectHolder Share(Object& object);    // Создаёт ObjectHolder, не владеющий объектом (аналог слабой ссылки)
    [[nodiscard]] static ObjectHolder None();    // Создаёт пустой ObjectHolder, соответствующий значению None

    Object& operator*() const;    // Возвращает ссылку на Object внутри ObjectHolder (должен быть непустым).
    Object* operator->() const;

    [[nodiscard]] Object* Get() const;

    template <typename T>    // Возвращает указатель на объект типа T либо nullptr, если внутри ObjectHolder не хранится объект данного типа
    [[nodiscard]] T* TryAs() const {
        return dynamic_cast<T*>(this->Get());
    }

    explicit operator bool() const;    // Возвращает true, если ObjectHolder не пуст

private:
    explicit ObjectHolder(std::shared_ptr<Object> data);
    void AssertIsValid() const;

    std::shared_ptr<Object> data_;
};

template <typename T>    // Объект-значение, хранящий значение типа T
class ValueObject : public Object {
public:
    ValueObject(T v)  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            : value_(v) {
    }

    void Print(std::ostream& os, [[maybe_unused]] Context& context) override {
        os << value_;
    }

    [[nodiscard]] const T& GetValue() const {
        return value_;
    }

private:
    T value_;
};

using Closure = std::unordered_map<std::string, ObjectHolder>;    // Таблица символов, связывающая имя объекта с его значением

bool IsTrue(const ObjectHolder& object);    // Проверяет, содержится ли в object значение, приводимое к True
                                            // для отличных от нуля чисел, True и непустых строк возвращается true. В остальных случаях - false.
class Executable {    // Интерфейс для выполнения действий над объектами Mython
public:
    virtual ~Executable() = default;
    virtual ObjectHolder Execute(Closure& closure, Context& context) = 0;    // Выполняет действие над объектами внутри closure, используя context
};                                                                           // Возвращает результирующее значение либо None

using String = ValueObject<std::string>;    // Строковое значение
using Number = ValueObject<int>;    // Числовое значение

class Bool : public ValueObject<bool> {    // Логическое значение
public:
    using ValueObject<bool>::ValueObject;

    void Print(std::ostream& os, Context& context) override;
};

struct Method {    // Метод класса
    std::string name;    // Имя метода
    std::vector<std::string> formal_params;    // Имена формальных параметров метода
    std::unique_ptr<Executable> body;    // Тело метода
};

class Class : public Object {    // Класс
public:
    explicit Class(std::string name, std::vector<Method> methods, const Class* parent);    // Создаёт класс с именем name и набором методов methods, унаследованный от класса parent
                                                                                            // Если parent равен nullptr, то создаётся базовый класс
    [[nodiscard]] const Method* GetMethod(const std::string& name) const;    // Возвращает указатель на метод name или nullptr, если метод с таким именем отсутствует

    [[nodiscard]] const std::string& GetName() const;    // Возвращает имя класса

    void Print(std::ostream& os, Context& context) override;    // Выводит в os строку "Class <имя класса>", например "Class cat"
private:
    const Class* parent_;
    std::string name_;
    std::vector<Method> methods_;
    std::unordered_map<std::string_view, const Method*> name_to_method_;
};

class ClassInstance : public Object {    // Экземпляр класса
public:
    explicit ClassInstance(const Class& cls);

    // Возвращает true, если объект имеет метод method, принимающий argument_count параметров
    [[nodiscard]] bool HasMethod(const std::string& method, size_t argument_count) const;
    /*
        * Вызывает у объекта метод method, передавая ему actual_args параметров.
        * Параметр context задаёт контекст для выполнения метода.
        * Если ни сам класс, ни его родители не содержат метод method, метод выбрасывает исключение
        * runtime_error
        */
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                        Context& context);
    /*
        * Если у объекта есть метод __str__, выводит в os результат, возвращённый этим методом.
        * В противном случае в os выводится адрес объекта.
        */
    void Print(std::ostream& os, Context& context) override;

    [[nodiscard]] Closure& Fields();    // Возвращает ссылку на Closure, содержащий поля объекта

    [[nodiscard]] const Closure& Fields() const;    // Возвращает константную ссылку на Closure, содержащую поля объекта

private:
    const Class& cls_;
    Closure fields_;
};

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

struct DummyContext : Context {
    std::ostream& GetOutputStream() override {
        return output;
    }

    std::ostringstream output;
};

class SimpleContext : public runtime::Context {    // Простой контекст, в нём вывод происходит в поток output, переданный в конструктор
public:
    explicit SimpleContext(std::ostream& output)
            : output_(output) {
    }

    std::ostream& GetOutputStream() override {
        return output_;
    }

private:
    std::ostream& output_;
};

}  // namespace runtime
