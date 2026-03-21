/*
Модуль: app.js
Назначение: Клиентская логика СУТ
Автор: Разработчик
Дата создания: 20.03.2026
Требования: NFR-01
*/

/* Автоскрытие алертов через 5 секунд */
document.addEventListener('DOMContentLoaded', function() {
    var alerts = document.querySelectorAll('.alert-dismissible');
    alerts.forEach(function(alert) {
        setTimeout(function() {
            var bsAlert = bootstrap.Alert.getOrCreateInstance(alert);
            bsAlert.close();
        }, 5000);
    });
});
