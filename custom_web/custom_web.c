    #include "esp_http_server.h"
    #include "tcpip_adapter.h"
    #include "lwip/ip_addr.h"
    #include "esp_attr.h"

    #define VER "1"

    char sparam1[20];
    char sparam2[20];
    int32_t iparam1; 
    int32_t iparam2;
    float   fparam1;
    float   fparam2;
    uint8_t checkbox;

#define NVS_SPACE "myweb"

static const char* WTAG = "WEB";

#define ICACHE_RODATA_ATTR __attribute__((aligned(4)))

const char *html_page_form_data ICACHE_RODATA_ATTR = "<div>"
                                    "<h4>Parameters:</h4>"
                                    "<form method='GET'>"
                                        "<p><label>checkbox (bool): </label><input type='checkbox' name='checkbox' %s /></p>"
                                        "<p><label>sparam1 (char): </label><input size='20' name='sparam1' value='%s' /></p>"
                                        "<p><label>sparam2 (char): </label><input size='20' name='sparam2' value='%s' /></p>"
                                        "<p><label>iparam1 (int32_t): </label><input size='20' name='iparam1' value='%d' /></p>"
                                        "<p><label>iparam2 (int32_t): </label><input size='20' name='iparam2' value='%d' /></p>"
                                        "<p><label>fparam1 (float): </label><input size='20' name='fparam1' value='%.02f' /></p>"
                                        "<p><label>fparam2 (float): </label><input size='20' name='fparam2' value='%.02f' /></p>"
                                        "<p><input type='hidden' name='st' value='my'></p>"  
                                        "<p><input type='submit' value='Save'></p>"
                                    "</form>"
                                    "</div>";  
// ******************************************************************************
// ********** ФУНКЦИИ ДЛЯ РАБОТЫ С NVS *****************************************
// ******************************************************************************
esp_err_t nvs_param_load(const char* space_name, const char* key, void* dest)
{
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    nvs_handle my_handle;
    size_t required_size = 0;
    ret = nvs_open(space_name, NVS_READWRITE, &my_handle);
    ret = nvs_get_blob(my_handle, key, NULL, &required_size);
    if (required_size == 0) {
        ESP_LOGW("NVS", "the target [ %s ] you want to load has never been saved", key);
        ret = ESP_FAIL;
        goto LOAD_FINISH;
    }
    ret = nvs_get_blob(my_handle, key, dest, &required_size);

  LOAD_FINISH:
    nvs_close(my_handle);

  OPEN_FAIL:
    return ret;
}

esp_err_t nvs_param_save(const char* space_name, const char* key, void *param, uint16_t len)
{
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    nvs_handle my_handle;
    ret = nvs_open(space_name, NVS_READWRITE, &my_handle);
    ret = nvs_set_blob(my_handle, key, param, len);
    ret = nvs_commit(my_handle);

  SAVE_FINISH:
    nvs_close(my_handle);

  OPEN_FAIL:
    return ret;
}




esp_err_t http_get_has_params(httpd_req_t *req){
    return ( httpd_req_get_url_query_len(req) > 0 ) ? ESP_OK : ESP_FAIL;
}

esp_err_t http_get_key_str(httpd_req_t *req, const char *param_name, char *value, size_t size){
    // get params 
    esp_err_t error = ESP_FAIL;
    char*  buf;
    size_t buf_len;
    
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len <= 1) return error;
    
    buf = malloc(buf_len);
    error = httpd_req_get_url_query_str(req, buf, buf_len);
    if ( error == ESP_OK) {
        ESP_LOGD(WTAG, "Found URL query => %s", buf);
        /* Get value of expected key from query string */
        error = httpd_query_key_value(buf, param_name, value, size);
        if ( error == ESP_OK) 
                ESP_LOGD(WTAG, "Found URL query parameter => %s=%s", param_name, value);
        //else ESP_LOGD(WTAG, esp_err_to_name( error ));
    }       
    free(buf);
    return error;
}

void httpd_resp_sendstr_chunk(httpd_req_t *req, const char *buf){
    httpd_resp_send_chunk(req, buf, strlen(buf));
}


void httpd_resp_sendstr_chunk_fmt(httpd_req_t *req, const char *fmt, ...){
    char *str = (char *) malloc(100);
    memset(str, 0, 100);

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(str, 100, fmt, args);
    va_end(args);

    str = (char *) realloc(str, len + 1);
    memset(str, 0, len + 1);
    len = vsnprintf(str, len + 1, fmt, args);

    httpd_resp_send_chunk(req, str, len + 1);

    free(str);
	str = NULL;
}

char* copy_str_from_str(const char *str, const char *str2)
{
    char *p = strstr(str, str2);
    uint8_t pos = p - str;
    p = (char *) calloc(1, pos + 1);
    strncpy(p, str, pos);
    return p;
}

char *http_uri_clean(httpd_req_t *req)
{
    char *p;
    if ( http_get_has_params(req) == ESP_OK) 
	{
        p = copy_str_from_str( req->uri, "?");
    } else {
        p = (char *) calloc(1, strlen( req->uri));
        strcpy(p, req->uri);
    }
    return p;  
}

void make_redirect(httpd_req_t *req, uint8_t timeout, const char *path) {


        tcpip_adapter_ip_info_t local_ip;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);

    char ip[30];
    sprintf(ip,"http://%d.%d.%d.%d:8080" , IP2STR( &local_ip.ip) ); 

    ESP_LOGI( WTAG, "%s: redirect to %s%s", __func__, ip, path);
    char t[4];
    itoa(timeout, t, 10);
    char *hdr = calloc(1, strlen(t) + 2 + strlen(path) + 1 + 30);
    strcpy(hdr, t);
    strcat(hdr, "; ");
    strcat(hdr, ip);
    strcat(hdr, path);
    httpd_resp_set_hdr(req, "Refresh", hdr);

    ESP_LOGI( WTAG, "%s: header %s", __func__, hdr);
    httpd_resp_send(req, NULL, 0);
    free(hdr);
}

    //extern httpd_handle_t serverhttp;
httpd_handle_t server = NULL;
    esp_err_t hello_get_handler(httpd_req_t *req)
    {
        //char resp[200]; 
        
        if ( http_get_has_params(req) == ESP_OK) 
        {
            // обработка входящих параметров, в т.ч. и ajax для главной
            char param[100];
            // проверяем наличие идентифицируещего параметра &st=1 или другое число или имя, т.е. идентификатор запроса
            if ( http_get_key_str(req, "st", param, sizeof(param)) == ESP_OK ) {
            
                if ( strcmp(param, "my") != 0 ) {
                    // параметры запроса есть, но идентифицирующего параметра нет
                    return ESP_FAIL;	
                }
            }    

            if ( http_get_key_str(req, "sparam1", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive sparam1 = %s", __func__,  param);
                strncpy(sparam1, param, 20);

                nvs_param_save(NVS_SPACE, "sparam1", &sparam1, strlen(sparam1));
            }      

            if ( http_get_key_str(req, "sparam2", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive sparam2 = %s", __func__,  param);
                strncpy(sparam2, param, 20);
                nvs_param_save(NVS_SPACE, "sparam2", &sparam2, strlen(sparam2));
            }   

            if ( http_get_key_str(req, "iparam1", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive iparam1 = %s", __func__,  param);
                iparam1 = atoi(param);
                nvs_param_save(NVS_SPACE, "iparam1", &iparam1, sizeof(iparam1));
            }  

            if ( http_get_key_str(req, "iparam2", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive iparam2 = %s", __func__,  param);
                iparam2 = atoi(param);
                nvs_param_save(NVS_SPACE, "iparam2", &iparam2, sizeof(iparam2));
            }   

            if ( http_get_key_str(req, "fparam1", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive fparam1 = %s", __func__,  param);
                fparam1 = atof(param);
                nvs_param_save(NVS_SPACE, "fparam1", &fparam1, sizeof(fparam1));
            }   

            if ( http_get_key_str(req, "fparam2", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive fparam2 = %s", __func__,  param);
                fparam2 = atof(param);
                nvs_param_save(NVS_SPACE, "fparam2", &fparam2, sizeof(fparam2));
            }  

            if ( http_get_key_str(req, "checkbox", param, sizeof(param)) == ESP_OK ) 
            {  
                ESP_LOGI( WTAG, "%s: receive checkbox = %s", __func__,  param);
                checkbox = 1;
                nvs_param_save(NVS_SPACE, "checkbox", &checkbox, sizeof(checkbox));
            }  else {
                checkbox = 0;
                nvs_param_save(NVS_SPACE, "checkbox", &checkbox, sizeof(checkbox));
            }
            // redirect to main page
            //httpd_resp_set_hdr(req, "Refresh", "0; /");

            // clean uri
            char *path = http_uri_clean( req );

            make_redirect(req, 0, path);
            free( path );
            return ESP_OK;

            //return ESP_OK;
        } 
        
        
            //нет параметров, выводим страничку для браузера
            httpd_resp_sendstr_chunk_fmt(req, html_page_form_data
                                                 , checkbox ? "checked" : ""
                                                 , sparam1
                                                 , sparam2
                                                 , iparam1
                                                 , iparam2
                                                 , fparam1
                                                 , fparam2
            );
            httpd_resp_sendstr_chunk(req, "<div><a href='/'>Main page</a></div>");
    

        //httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	    //httpd_resp_send(req, resp, strlen(resp)); 
        httpd_resp_send_chunk(req, NULL, 0);

        return ESP_OK;
    }

    httpd_uri_t hello = {
        .uri      = "/hello",
        .method   = HTTP_GET,
        .handler  = hello_get_handler,
        .user_ctx = NULL
    };

httpd_handle_t start_my_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port        = 8080;
    config.ctrl_port                  = 32767;
    /* Empty handle to esp_http_server */
    

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        ESP_LOGI(WTAG, "try to httpd_register_uri_handler");
        esp_err_t err = httpd_register_uri_handler(server, &hello);
        ESP_LOGI(WTAG, "uri_fuel_get registred %s", err == ESP_OK ? "SUCCESS" : "FAIL");
    }
    /* If server failed to start, handle will be NULL */
    return server;
}


    void
    startfunc(){
    // выполняется один раз при старте модуля.

        ESP_LOGI(WTAG, "Starting webserver");
        start_my_webserver();

        nvs_param_load(NVS_SPACE, "checkbox", &checkbox);
        nvs_param_load(NVS_SPACE, "sparam1", &sparam1);
        nvs_param_load(NVS_SPACE, "sparam2", &sparam2);
        nvs_param_load(NVS_SPACE, "iparam1", &iparam1);
        nvs_param_load(NVS_SPACE, "iparam2", &iparam2);
        nvs_param_load(NVS_SPACE, "fparam1", &fparam1);
        nvs_param_load(NVS_SPACE, "fparam2", &fparam2);

    }

    void
    timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду
    if(timersrc%30==0){
    // выполнение кода каждые 30 секунд
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    void webfunc(char *pbuf) {
//     os_sprintf(HTTPBUFF,"
// <head>
// <meta http-equiv='refresh' content='1;URL=http://ab-w.net" />
// </head>", VER); // вывод данных на главной модуля


    os_sprintf(HTTPBUFF,"<br>test httpd_register_uri_handler %s", VER); 


        tcpip_adapter_ip_info_t local_ip;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);


    os_sprintf(HTTPBUFF,"<br>ip addr %d.%d.%d.%d" , IP2STR( &local_ip.ip) ); 
    os_sprintf(HTTPBUFF,"<br><a href='http://%d.%d.%d.%d:8080%s'>Hello page</a>", IP2STR( &local_ip.ip), hello.uri); 

        os_sprintf(HTTPBUFF,"<br><h2>Параметры:</h2>"); 
        os_sprintf(HTTPBUFF,"<br>checkbox (bool): <b>%s</b>", checkbox ? "true" : "false"); 
        os_sprintf(HTTPBUFF,"<br>sparam1 (char): <b>%s</b>", sparam1); 
        os_sprintf(HTTPBUFF,"<br>sparam2 (char): <b>%s</b>", sparam2); 
        os_sprintf(HTTPBUFF,"<br>iparam1 (int32_t): <b>%d</b>", iparam1); 
        os_sprintf(HTTPBUFF,"<br>iparam2 (int32_t): <b>%d</b>", iparam2); 
        os_sprintf(HTTPBUFF,"<br>fparam1 (float): <b>%.2f</b>", fparam1); 
        os_sprintf(HTTPBUFF,"<br>fparam2 (float): <b>%.2f</b>", fparam2); 

    //os_sprintf(HTTPBUFF,"<br><meta http-equiv='refresh' content='0; url=http://%d.%d.%d.%d:8080/hello'>" , IP2STR( &local_ip.ip) ); // вывод данных на главной модуля
    }