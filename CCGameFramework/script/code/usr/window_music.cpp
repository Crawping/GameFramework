#include "/include/io"
#include "/include/window"
#include "/include/fs"
#include "/include/memory"
#include "/include/string"
#include "/include/xtoa_itoa"
#include "/include/proc"
#include "/include/readfile"
#include "/include/json"
void play(char *name);
int read_file(int id, int handle) {
    int c;
    window_layout_linear_set_vertical_align(window_get_base(id));
    window_set_style(id, style_win10_white);
    long text = window_create_comctl(id, comctl_label);
    long text2 = window_create_comctl(id, comctl_button);
    long text3 = window_create_comctl(id, comctl_button);
    long text4 = window_create_comctl(id, comctl_button);
    window_comctl_connect(window_get_base(id), text);
    window_comctl_connect(window_get_base(id), text2);
    window_comctl_connect(window_get_base(id), text3);
    window_comctl_connect(window_get_base(id), text4);
    window_comctl_set_text(text, "歌名");
    window_comctl_set_text(text2, "播放");
    window_comctl_set_text(text3, "");
    window_comctl_set_text(text4, "");
    window_comctl_set_bound(text, 10, 10, 300, 30);
    window_comctl_set_bound(text2, 10, 10, 200, 30);
    window_comctl_set_bound(text3, 10, 10, 200, 30);
    window_comctl_set_bound(text4, 10, 10, 200, 30);
    window_comctl_label_set_horizontal_align_middle(text);
    window_comctl_label_set_horizontal_align_middle(text2);
    window_comctl_label_set_horizontal_align_middle(text3);
    window_comctl_label_set_horizontal_align_middle(text4);
    int t2id = window_get_comctl(text2);
    __window_msg_struct__ s;
    while (c = window_get_msg(handle, &s), c < 0x1000) {
        window_default_msg(id, &s);
        if (s.code == 0x201) {
            if (s.comctl == t2id) {
                if (fork() == -1) {
                    play("New World");
                    exit(0);
                }
            }
        }
    }
    switch (c) {
    case 0x2000:
        // put_string("[INFO] Read to the end.");
        break;
    case 0x2001:
        set_fg(240, 0, 0);
        put_string("[ERROR] Read error.");
        restore_fg();
        break;
    }
    close(handle);
    close(id);
}
int main(int argc, char **argv) {
    __window_create_struct__ s;
    s.caption = "在线听歌";
    s.left = 50;
    s.top = 50;
    s.width = 400;
    s.height = 300;
    int id = window_create(&s);
    put_string("Create test window: ");
    put_hex(id);
    put_string("\n");
    char* msg = malloc(20);
    strcpy(msg, "/handle/");
    char* fmt = malloc(20);
    i32toa(id, fmt);
    strcat(msg, fmt);
    strcat(msg, "/message");
    int handle = open(msg);
    switch (handle) {
        default:
            // put_string("[INFO] Success.");
            read_file(id, handle);
            break;
        case -1:
            set_fg(240, 0, 0);
            put_string("[ERROR] File not exists.");
            restore_fg();
            break;
        case -2:
            set_fg(240, 0, 0);
            put_string("[ERROR] Path is not file.");
            restore_fg();
            break;
        case -3:
            set_fg(240, 0, 0);
            put_string("[ERROR] File is locked.");
            restore_fg();
            break;
    }
    //window_wait(id);
    return 0;
}

void play(char* name) {
    char* path = malloc(100);
    strcpy(path, "/http/post!music.163.com/api/search/suggest/web!s=");
    strcat(path, name);
    put_string("Open: "); put_string(path); put_string("\n");
    char* json; int len;
    if (readfile(path, &json, &len) == 0) {
        json_object* obj = json_parse_obj(json);
        if (obj) {
            put_string("Code: ");
            int code = json_obj_get_string(obj, "code")->data.i;
            put_int(code);
            put_string("\n");
            if (code != 200) goto FAILED;
            json_object* result = json_obj_get_string(obj, "result");
            if (!result) goto FAILED;
            json_object* songs = json_obj_get_string(result, "songs");
            put_string("Count of songs: ");
            int N = json_array_size(songs);
            put_int(N);
            put_string("\n");
            int i;
            for (i = 0; i < N; i++) {
                put_string(" --> ["); put_int(i); put_string("] ");
                json_object* s = json_array_get(songs, i);
                put_string("id= ");
                put_int(json_obj_get_string(s, "id")->data.i);
                put_string(", name= ");
                put_string(json_obj_get_string(s, "name")->data.str);
                put_string("\n");
            }
        FAILED:
            free(obj);
        }
        else {
            put_string("Parsing json failed.\n");
        }
        free(json);
    }
    else {
        put_string("Open failed.\n");
    }
    free(path);
}