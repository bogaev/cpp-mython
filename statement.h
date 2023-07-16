#pragma once

#include "runtime.h"

#include <functional>

namespace ast {

using Statement = runtime::Executable;

template <typename T>    // Выражение, возвращающее значение типа T, используется как основа для создания констант
class ValueStatement : public Statement {
public:
    explicit ValueStatement(T v)
            : value_(std::move(v)) {
    }

    runtime::ObjectHolder Execute(runtime::Closure& /*closure*/,
                                    runtime::Context& /*context*/) override {
        return runtime::ObjectHolder::Share(value_);
    }

private:
    T value_;
};

using NumericConst = ValueStatement<runtime::Number>;
using StringConst = ValueStatement<runtime::String>;
using BoolConst = ValueStatement<runtime::Bool>;

/*
Вычисляет значение переменной либо цепочки вызовов полей объектов id1.id2.id3.
Например, выражение circle.center.x - цепочка вызовов полей объектов в инструкции:
x = circle.center.x
*/
class VariableValue : public Statement {
public:
    explicit VariableValue(const std::string& var_name);
    explicit VariableValue(std::vector<std::string> dotted_ids);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
private:
    std::vector<std::string> dotted_ids_;
};

class Assignment : public Statement {   // Присваивает переменной, имя которой задано в параметре var, значение выражения rv
public:
    Assignment(std::string var, std::unique_ptr<Statement> rv);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::string var_;
    std::unique_ptr<Statement> rv_;
};

class BinaryOperation : public Statement {    // Родительский класс Бинарная операция с аргументами lhs и rhs
public:
    BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);

protected:
    std::unique_ptr<Statement> lhs_;
    std::unique_ptr<Statement> rhs_;
};

class ClassDefinition : public Statement {    // Объявляет класс
public:
    // Гарантируется, что ObjectHolder содержит объект типа runtime::Class
    explicit ClassDefinition(runtime::ObjectHolder cls);

    // Создаёт внутри closure новый объект, совпадающий с именем класса и значением, переданным в конструктор
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    runtime::ObjectHolder cls_;
};

class Compound : public Statement {    // Составная инструкция (например: тело метода, содержимое ветки if, либо else)
public:
    template <typename... Args>    // Конструирует Compound из нескольких инструкций типа unique_ptr<Statement>
    explicit Compound(Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            CompoundArgs(args...);
        }
    }

    void AddStatement(std::unique_ptr<Statement> stmt);    // Добавляет очередную инструкцию в конец составной инструкции

    // Последовательно выполняет добавленные инструкции. Возвращает None
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    template <typename T0, typename... Ts>
    void CompoundArgs(T0&& v0, Ts&&... vs) {
        if constexpr (sizeof...(vs) != 0) {
            statements_.push_back(std::move(v0));
            CompoundArgs(vs...);
        } else {
            statements_.push_back(std::move(v0));
        }
    }

private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

class FieldAssignment : public Statement {    // Присваивает полю object.field_name значение выражения rv
public:
    FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    VariableValue object_;
    std::string field_name_;
    std::unique_ptr<Statement> rv_;
};

class IfElse : public Statement {    // Инструкция if <condition> <if_body> else <else_body>
public:
    IfElse(std::unique_ptr<Statement> condition,
            std::unique_ptr<Statement> if_body,
            std::unique_ptr<Statement> else_body);    // Параметр else_body может быть равен nullptr

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::unique_ptr<Statement> condition_;
    std::unique_ptr<Statement> if_body_;
    std::unique_ptr<Statement> else_body_;
};

class MethodBody : public Statement {    // Тело метода. Как правило, содержит составную инструкцию
public:
    explicit MethodBody(std::unique_ptr<Statement>&& body);

    // Вычисляет инструкцию, переданную в качестве body.
    // Если внутри body была выполнена инструкция return, возвращает результат return
    // В противном случае возвращает None
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::unique_ptr<Statement> body_;
};

class MethodCall : public Statement {    // Вызывает метод object.method со списком параметров args
public:
    MethodCall(std::unique_ptr<Statement> object,
                std::string method,
                std::vector<std::unique_ptr<Statement>> args);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::unique_ptr<Statement> object_;
    std::string method_name_;
    std::vector<std::unique_ptr<Statement>> args_;
};

/*
Создаёт новый экземпляр класса class_, передавая его конструктору набор параметров args.
Если в классе отсутствует метод __init__ с заданным количеством аргументов,
то экземпляр класса создаётся без вызова конструктора (поля объекта не будут проинициализированы):

class Person:
def set_name(name):
self.name = name

p = Person()
# Поле name будет иметь значение только после вызова метода set_name
p.set_name("Ivan")
*/
class NewInstance : public Statement {
public:
    explicit NewInstance(const runtime::Class& class_);
    NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);
    // Возвращает объект, содержащий значение типа ClassInstance
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    runtime::ClassInstance class_inst_;
    std::vector<std::unique_ptr<Statement>> args_;
};

class None : public Statement {    // Значение None
public:
    runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure& /* closure */,
                                    [[maybe_unused]] runtime::Context& /* context */) override {
        return {};
    }
};

class Print : public Statement {    // Команда print
public:
    explicit Print(std::unique_ptr<Statement> argument);    // Инициализирует команду print для вывода значения выражения argument
    explicit Print(std::vector<std::unique_ptr<Statement>> args);    // Инициализирует команду print для вывода списка значений args

    static std::unique_ptr<Print> Variable(const std::string& name);    // Инициализирует команду print для вывода значения переменной name

    // Во время выполнения команды print вывод должен осуществляться в поток, возвращаемый из context.GetOutputStream()
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::vector<std::unique_ptr<Statement>> args_;
};

class Return : public Statement {    // Выполняет инструкцию return с выражением statement
public:
    explicit Return(std::unique_ptr<Statement> statement);

    // Останавливает выполнение текущего метода. После выполнения инструкции return метод,
    // внутри которого она была исполнена, должен вернуть результат вычисления выражения statement.
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    std::unique_ptr<Statement> statement_;
};

class UnaryOperation : public Statement {    // Базовый класс для унарных операций
public:
    explicit UnaryOperation(std::unique_ptr<Statement> argument);

protected:
    std::unique_ptr<Statement> argument_;
};

class Not : public UnaryOperation {    // Возвращает результат вычисления логической операции not над единственным аргументом операции
public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Stringify : public UnaryOperation {    // Операция str, возвращающая строковое значение своего аргумента
public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Add : public BinaryOperation {    // Возвращает результат операции + над аргументами lhs и rhs
public:
    using BinaryOperation::BinaryOperation;

    // Поддерживается сложение: число + число, строка + строка
    //  объект1 + объект2, если у объект1 - пользовательский класс с методом _add__(rhs)
    // В противном случае при вычислении выбрасывается runtime_error
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class And : public BinaryOperation {    // Возвращает результат вычисления логической операции and над lhs и rhs
public:
    using BinaryOperation::BinaryOperation;
    // Значение аргумента rhs вычисляется, только если значение lhs после приведения к Bool равно True
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Comparison : public BinaryOperation {    // Операция сравнения
public:
    // Comparator задаёт функцию, выполняющую сравнение значений аргументов
    using Comparator = std::function<bool(const runtime::ObjectHolder&,
                                            const runtime::ObjectHolder&,
                                            runtime::Context&)>;

    Comparison(Comparator cmp, std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);

    // Вычисляет значение выражений lhs и rhs и возвращает результат работы comparator, приведённый к типу runtime::Bool
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

private:
    Comparator cmp_;
};

class Div : public BinaryOperation {    // Возвращает результат деления lhs и rhs
public:
    using BinaryOperation::BinaryOperation;

    // Поддерживается деление: число / число
    // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    // Если rhs равен 0, выбрасывается исключение runtime_error
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Mult : public BinaryOperation {    // Возвращает результат умножения аргументов lhs и rhs
public:
    using BinaryOperation::BinaryOperation;

    // Поддерживается умножение: число * число
    // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Or : public BinaryOperation {    // Возвращает результат вычисления логической операции or над lhs и rhs
public:
    using BinaryOperation::BinaryOperation;
    // Значение аргумента rhs вычисляется, только если значение lhs после приведения к Bool равно False
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Sub : public BinaryOperation {    // Возвращает результат вычитания аргументов lhs и rhs
public:
    using BinaryOperation::BinaryOperation;

    // Поддерживается вычитание: число - число. Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class ReturnException : public std::exception {
public:
    explicit ReturnException(runtime::ObjectHolder&& obj)
            : obj_(obj) {
    }

    runtime::ObjectHolder GetValue() {
        return obj_;
    }

private:
    runtime::ObjectHolder obj_;
};

}  // namespace ast