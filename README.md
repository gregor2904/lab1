# lab1

Некоторые пояснения по принятым решениям.

1. У класса видны 2 сценария использования, с подписчиком и без.
Поэтому при наличии подписчика на очередь - запрещен явный выбор из очереди.

2. Решил не заводить отдельную нить для разбора очередей клиентами. 
Не вижу в этом смысла. При подписке клиент сразу вычитывает всю очередь. При вставке в очередь, если есть подписчик - значение сразу уходит к нему. Этого достаточно.

3. Поскольку метод выбора из очереди обязан возвращать объект - пришлось заводить исключение на отсутствие объекта (аналогично std::vector::at(), например)
Возвращать созданный дефолтный объект неправильно.

4. Если потребуется сделать несколько подписчиков у одной очереди, это сделать совсем несложно.
Требуется заменить хранение указателя на хранение контейнера указателей в реализации и немного причесать код.

5. Код еще можно причесать и облагородить, но я обещал отдать решение сегодня. Потрачено суммарно часа 4 на все. Надеюсь ход мыслей понятен.

6. Решение в виде проекта MSVS консольного приложения для Windows.
Проверял также в https://www.onlinegdb.com, все работает и там.
