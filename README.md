# 🐍 C++ интерпретатор языка Mython
**Mython** (Mini-python) — интерпретатор упрощённого подмножества языка Python, реализованный на базе C++.

Поддерживает следующие элементы языка:

<details>
<summary>Числа, строки, логические константы, комментарии, идентификаторы, семантика присваивания</summary>

| | |
| ---- | ---- |
| **Исходный код** | `x = 57`<br>`print(x)`<br>`x = 'C++ black belt'`<br>`print(x)`<br>`y = False`<br>`x = y`<br>`print(x)`<br>`x = None`<br>`print(x, y)` |
| **Вывод** | `57`<br>`C++ black belt`<br>`False`<br>`None False` |
</details>
<details>
<summary>Структуры данных, типизация, встроенная функция str, print</summary>

  - *Классы*
  - *Наследование*
  - *Методы*

	| | |
	| ---- | ---- |
	| **Исходный код** | `class Shape:`<br>`  def __str__(self):`<br>`    return "Shape"`<br><br>`class Rect(Shape):`<br>`  def __init__(self, w, h):`<br>`    self.w = w`<br>`    self.h = h`<br>`  def __str__(self):`<br>`    return "Rect(" + str(self.w) + 'x' + str(self.h) + ')'`<br><br>`class Circle(Shape):`<br>`  def __init__(self, r):`<br>`    self.r = r`<br>`  def __str__(self):`<br>`    return 'Circle(' + str(self.r) + ')'`<br><br>`class Triangle(Shape):`<br>`  def __init__(self, a, b, c):`<br>`    self.ok = a + b > c and a + c > b and b + c > a`<br>`    if (self.ok):`<br>`      self.a = a`<br>`      self.b = b`<br>`      self.c = c`<br>`  def __str__(self):`<br>`    if self.ok:`<br>`      return 'Triangle(' + str(self.a) + ', ' + str(self.b) + ', ' + str(self.c) + ')'`<br>`    else:`<br>`      return 'Wrong triangle'`<br><br>`r = Rect(10, 20)`<br>`c = Circle(52)`<br>`t1 = Triangle(3, 4, 5)`<br>`t2 = Triangle(125, 1, 2)`<br><br>`print(r, c, t1, t2)` |
	| **Вывод** | `Rect(10x20) Circle(52) Triangle(3, 4, 5) Wrong triangle` |
</details>
<details>
<summary>Контроль потока</summary>

  - *Условный оператор*

	| | |
	| ---- | ---- |
	| **Исходный код** | `x = 4`<br>`y = 5`<br>`if x > y:`<br>`  print("x > y")`<br>`else:`<br>`  print("x <= y")`<br>`if x > 0:`<br>`  if y < 0:`<br>`    print("y < 0")`<br>`  else:`<br>`    print("y >= 0")`<br>`else:`<br>`  print("x <= 0")` |
	| **Вывод** | `x <= y`<br>`y >= 0` |		
</details>

## ⚙️ Системные требования
- C++17 standard
- STL [^1]
- Cmake [^2] minimum version: 3.21

## 💡 Использование
- Соберите интерпретатор mython с помощью cmake в директории build:

	```
	cd [path-to-cpp-mython-main]\
	mkdir build && cd build
	cmake .. && make
	```

- Для запуска интерпретатора для исходного кода из файла ```[source_filepath]``` и вывода печати в файл ```[output_filepath]```:

	```./mython [source_filepath] [output_filepath]``` (пример для Ubuntu)

- Для вывода справочной информации:

	```./mython --help```

- Для вывода тестовой информации:

	```./mython --test```

## 👨‍💻 Пример
- Получим вывод для следующего исходного кода (файл [my_program.txt](my_program.txt)):

	```
	program_name = "Classes test"
	
	class Empty:
	  def __init__():
	    x = 0
	
	class Point:
	  def __init__(x, y):
	    self.x = x
	    self.y = y
	
	  def SetX(value):
	    self.x = value
	  def SetY(value):
	    self.y = value
	
	  def __str__():
	    return '(' + str(self.x) + '; ' + str(self.y) + ')'
	
	origin = Empty()
	origin = Point(0, 0)
	
	far_far_away = Point(10000, 50000)
	
	print program_name, origin, far_far_away, origin.SetX(1)
	```

- Запускаем интерпретатор с выводом в файл ```out.txt```:

	```./mython my_program.txt out.txt```

- Вывод mython (файл ```out```):

	```Classes test (0; 0) (10000; 50000) None```
	
<!--
## Примеры
-->

[^1]: https://en.wikipedia.org/wiki/Standard_Template_Library
[^2]: https://cmake.org/download/