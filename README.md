# Мониторинг отредактированных файлов

## Задание

Программе передается абсолютный путь к папке. Требуется на stdout писать все файлы, когда их 'редактируют'

## Что такое 'редактируют'?

Под 'редактрированием' будем понимать запись все события, улавливаемые `FAN_NOTIFY` и никакие другие.

## Build

```bash
$ make
```

## Run

```bash
$ sudo ./a.out /path/to/directory
```

## Example

Подготовка
```bash
## terminal 1 ##
$ cd
$ mkdir -p testdir/ddd
```

Запускаем
```bash
## terminal 2 ##
$ sudo ./a.out /home/user/testdir
```

'Работаем'
```bash
## terminal 1 ##
$ echo 123 > testdir/ddd/ff.txt
$ echo 456 >> testdir/ddd/ff.txt
$ cat testdir/ddd/ff.txt 
123
456
$ rm -rf testdir/ddd/ff.txt 
$ rm -rf testdir 
```

Вывод программы (целиком):
```bash
## terminal 2 ##
$ sudo ./a.out /home/user/testdir
[LOG] Press eny key to terminate
[LOG] Listening for events
[NOTIFY] File modified: '/home/user/testdir/ddd/ff.txt'
[NOTIFY] File modified: '/home/user/testdir/ddd/ff.txt'
[LOG] Whatched folder had been deleted!
[LOG] Shutting down...
[LOG] Listening for events stopped.
#
```

(да, при `echo` происходит модификация файла)

## Notes

1. fanotify может смотреть в родительские (и прочие) папки?
    - Потому что он смотрит не в директорию, а в `mount point`. И если указанный путь таковым не является, он смотрит в ближайший (по вложенности) `mount point`
    - `man fanotify`
    - `It only works that way when working on mounted directories. I did the following test:` [stackoverflow post](http://stackoverflow.com/questions/19528432/fanotify-recursivity-does-really-works/19543049#19543049)
    - Поэтому полагаемся на абсолютный путь и сравниваем строки
