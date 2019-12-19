/*  
    "медленный" шим
    управление мощностью нагрузки временным регулированием вместо фазового
    на временном отрезке включаем нагрузку на нужных участках
    например, берем временной участок и делим его на 10 частей
    для мощности в 50% включаем нагрузку на первых 5ти участках времени, на последующих 5и отключаем

    Желательно разделить равномерно. Если 50%, то через один
    Если 60%, то 6 частей работают, 4 части не работают
    если чередовать, то 2 раб - 2 отдых - 2 раб - 2 отдых - 2 рабы
    в идеале надо использовать MOC и прерывание для ловли перехода напряжения через 0, чтобы корректно управлять полупериодами

    http://elref.ru/forum/18-715-1
    В 1 сек при частоте сети 50Гц проходит 100 полуволн/полупериодов (положительных и отрицательных) если пропустить только 50 то и получите 50% мощьности в секунду.
    http://arduino.ru/forum/programmirovanie/algoritm-ravnomernogo-raspredeleniya

    1 полуволна - 10 мсек
    2 полуволны - 20 мсек.
    Т.е. полный период.

    ШИМ для SSR для управления водонагревателем надо медленный, 1 Гц, по умолчанию 500 Гц, это много
    Оптимально 5гц
    для систем асутп, и более того, климата - частота до 50Гц - достаточна. системы инерционны, и порой достаточно даже 10Гц.

    Управляем пропуском фаз, получается что то вроде очень медленного шима например 50 мс вкл 500 мс выкл, но не менее 20 мс иначе получишь свистопляску с непонятным результатом.

    #define t_pwm 1000//период медленного ШИМ`а
    //для симистора или твердотельного реле от 1000мс (для реле не менее 60000мс)    

    //выдаем рассчитанную мощность
    ten.blink(t_pwm,(t_pwm/100)*power);
    //(t_pwm/100)*power) - перевод рассчитанной мощности (0-100%) в продолжительность импульса
    }

    Вместо переключения на частоте 500 Гц, 1 кГц, 20 кГц и т. Д. Необходимо переключаться с частотой Гц, например, 0,25 Гц, или с интервалом времени 4 секунды. Также можно использовать более длительные временные интервалы, такие как 30 секунд, упомянутые в ответе Сферо.

*/