# 📘 Пошаговая инструкция: Загрузка проекта на GitHub

## Часть 1: Получение GitHub Personal Access Token (PAT)

### Шаг 1: Войдите в GitHub
1. Откройте https://github.com
2. Войдите в свой аккаунт (grifmax)

### Шаг 2: Откройте Settings
1. Нажмите на ваш аватар в правом верхнем углу
2. Выберите **Settings** (Настройки)

### Шаг 3: Developer settings
1. В левом меню прокрутите вниз
2. Найдите и нажмите **Developer settings** (Настройки разработчика)

### Шаг 4: Personal access tokens
1. В левом меню нажмите **Personal access tokens**
2. Выберите **Tokens (classic)** или **Fine-grained tokens**

**Рекомендация:** Используйте **Tokens (classic)** - проще для начала

### Шаг 5: Generate new token
1. Нажмите **Generate new token** → **Generate new token (classic)**
2. Возможно потребуется ввести пароль для подтверждения

### Шаг 6: Настройка токена

**Note (Название токена):**
```
Rectification Controller Token
```

**Expiration (Срок действия):**
- Выберите **90 days** (или **No expiration** для постоянного)

**Select scopes (Разрешения):**

✅ Обязательно отметьте:
- ✅ **repo** (полный доступ к репозиториям)
  - ✅ repo:status
  - ✅ repo_deployment
  - ✅ public_repo
  - ✅ repo:invite
  - ✅ security_events

⚠️ Не нужно отмечать другие разрешения для простой загрузки кода

### Шаг 7: Создание токена
1. Прокрутите вниз
2. Нажмите **Generate token** (зеленая кнопка)

### Шаг 8: ВАЖНО! Сохраните токен
1. ⚠️ **СКОПИРУЙТЕ ТОКЕН ПРЯМО СЕЙЧАС!**
2. Токен выглядит примерно так: `ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`
3. ⚠️ **ВЫ УВИДИТЕ ЕГО ТОЛЬКО ОДИН РАЗ!**

**Сохраните токен в безопасном месте:**
- Текстовый файл на компьютере
- Менеджер паролей
- Защищенная заметка

---

## Часть 2: Создание репозитория на GitHub

### Шаг 1: Создание нового репозитория
1. Откройте https://github.com/new
2. Или нажмите "+" в правом верхнем углу → **New repository**

### Шаг 2: Настройки репозитория

**Repository name:**
```
rectification-controller
```

**Description:**
```
Automated rectification column control system based on ESP32
```

**Visibility:**
- 🔒 Выберите **Private** (приватный репозиторий)

**Initialize repository:**
- ❌ НЕ отмечайте "Add a README file"
- ❌ НЕ отмечайте "Add .gitignore"
- ❌ НЕ отмечайте "Choose a license"

*Мы уже подготовили эти файлы*

### Шаг 3: Создание
1. Нажмите **Create repository** (зеленая кнопка)
2. ✅ Репозиторий создан!

---

## Часть 3: Загрузка проекта

### Вариант А: Через командную строку (рекомендуется)

#### 1. Откройте терминал
- **Windows:** Git Bash или Command Prompt
- **Mac/Linux:** Terminal

#### 2. Перейдите в папку проекта
```bash
cd путь/к/rectification-controller
```

#### 3. Инициализация Git
```bash
git init
```

#### 4. Добавление файлов
```bash
git add .
```

#### 5. Первый commit
```bash
git commit -m "Initial commit: Rectification Controller v1.0.0"
```

#### 6. Добавление удаленного репозитория
```bash
git remote add origin https://github.com/grifmax/rectification-controller.git
```

#### 7. Загрузка на GitHub
```bash
git push -u origin main
```

**При запросе авторизации:**
- Username: `grifmax`
- Password: `ВАШ_ТОКЕН` (не пароль аккаунта, а PAT!)

---

### Вариант Б: Через GitHub Desktop (проще для новичков)

#### 1. Установите GitHub Desktop
- Скачайте: https://desktop.github.com
- Установите и войдите в аккаунт

#### 2. Добавление репозитория
1. File → Add local repository
2. Выберите папку `rectification-controller`
3. Нажмите "Add repository"

#### 3. Первый commit
1. В списке изменений увидите все файлы
2. Внизу в поле "Summary" введите:
   ```
   Initial commit: Rectification Controller v1.0.0
   ```
3. Нажмите **Commit to main**

#### 4. Публикация
1. Нажмите **Publish repository** вверху
2. Убедитесь что:
   - Name: `rectification-controller`
   - Description: указано
   - ✅ Keep this code private (отмечено!)
3. Нажмите **Publish Repository**

---

### Вариант В: Через веб-интерфейс GitHub (не рекомендуется)

Можно загружать файлы через "Upload files", но это неудобно для больших проектов.

---

## Часть 4: Проверка загрузки

### 1. Откройте репозиторий
```
https://github.com/grifmax/rectification-controller
```

### 2. Проверьте что есть:
- ✅ README.md отображается
- ✅ Все папки (src, test)
- ✅ platformio.ini
- ✅ LICENSE
- ✅ .gitignore
- ✅ Около 46 файлов

### 3. Проверьте README
- Должен красиво отображаться с форматированием
- Иконки должны быть видны

---

## 🎉 Готово!

Ваш проект успешно загружен на GitHub!

### Что дальше?

**Клонирование на другой компьютер:**
```bash
git clone https://github.com/grifmax/rectification-controller.git
```

**Обновление после изменений:**
```bash
git add .
git commit -m "Описание изменений"
git push
```

**Получение обновлений:**
```bash
git pull
```

---

## ⚠️ Безопасность токена

### ✅ Правильно:
- Хранить токен в безопасном месте
- Использовать менеджер паролей
- Установить срок действия токена

### ❌ НИКОГДА:
- ❌ Не публикуйте токен в коде
- ❌ Не сохраняйте в публичных местах
- ❌ Не отправляйте токен другим людям
- ❌ Не коммитьте файлы с токенами

### Если токен скомпрометирован:
1. Зайдите в Settings → Developer settings
2. Personal access tokens → Tokens (classic)
3. Найдите ваш токен
4. Нажмите **Delete** (красная кнопка)
5. Создайте новый токен

---

## 🆘 Решение проблем

### Проблема: "Permission denied"
**Решение:** Проверьте что используете токен, а не пароль

### Проблема: "Repository not found"
**Решение:** Проверьте URL репозитория и ваши права доступа

### Проблема: "Authentication failed"
**Решение:** 
1. Токен просрочен - создайте новый
2. Неправильный токен - проверьте что скопировали полностью
3. Нет прав - добавьте scope "repo"

### Проблема: Git не установлен
**Решение:** 
- Windows: https://git-scm.com/download/win
- Mac: `brew install git`
- Linux: `sudo apt install git`

---

## 📞 Нужна помощь?

**Документация GitHub:**
- https://docs.github.com/en/get-started

**Документация Git:**
- https://git-scm.com/doc

**Туториалы:**
- https://learngitbranching.js.org (интерактивный)

---

## ✅ Чеклист

Проверьте что выполнили:

- [ ] Создан GitHub Personal Access Token
- [ ] Токен сохранен в безопасном месте
- [ ] Создан приватный репозиторий на GitHub
- [ ] Проект загружен в репозиторий
- [ ] README.md отображается корректно
- [ ] Все файлы на месте
- [ ] Git настроен локально

---

**Дата создания инструкции:** 22 ноября 2025  
**Версия:** 1.0

🎊 **Поздравляем! Ваш проект на GitHub!** 🎊
