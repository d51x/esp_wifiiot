char** str_split(char* a_str, const char a_delim, int *cnt)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    *cnt = count;
    result = malloc(sizeof(char*) * count);
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(NULL, delim);
        }
        *(result + idx) = 0;
    }
    return result;
}

uint8_t buzzer_parse(const char *pattern, buzzer_beep_t *buzzer_beeps)
{
    ESP_LOGI(UTAG, "%s: pattern = %s", __func__, pattern);
    char** beeps;
    uint8_t count = 0;
    //buzzer_beep_t *buzzer_beeps;
    
    char *str = strdup(pattern);
    beeps = str_split(str, ';', &count);
    if (beeps)
    {
        buzzer_beep_t *data = malloc( count * sizeof(buzzer_beep_t ) );
        buzzer_beep_t *buzzer_beeps = malloc( count * sizeof(buzzer_beep_t ) );
        int i;
        for (i = 0; *(beeps + i); i++)
        {
            char *s1 = strchr(*(beeps + i), ':');
            if ( s1 != NULL ) {
                char t[5] = "";
                strncpy(t,*(beeps + i),s1-*(beeps + i));
                (data+i)->action = atoi(t);
                strcpy(t,*(beeps + i) + 2);
                (data+i)->delay = atoi(t);
            }
            free(*(beeps + i));
        }
        
        ESP_LOGI(UTAG, "%s: memcpy", __func__);
        memcpy(buzzer_beeps, data, count * sizeof(buzzer_beep_t ));

        ESP_LOGI(UTAG, "%s: free(data)", __func__);
        free(data);

        ESP_LOGI(UTAG, "%s: free(beeps)", __func__);
        free(beeps);
        
        
    }
    ESP_LOGI(UTAG, "%s: free(str) = %s", __func__, str);
    free(str);
    ESP_LOGI(UTAG, "%s: count = %d", __func__, count);
    return count;
}