#include <gtk/gtk.h>
#include <seccomp.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

GtkWidget *process_list_view;
GtkWidget *confirm_dialog;

typedef struct {
    gchar *pid;
    gchar *name;
    gchar *state;
    gchar *memory;
} ProcessInfo;

void update_process_list(GtkListStore *store) {
    gtk_list_store_clear(store);

    DIR *dir = opendir("/proc");
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && atoi(entry->d_name) != 0) {
                gchar *pid = g_strdup_printf("%s", entry->d_name);
                gchar *proc_dir = g_strdup_printf("/proc/%s", entry->d_name);

                gchar *name = NULL, *state = NULL, *memory = NULL;

                // 读取进程信息
                FILE *stat_file = fopen(g_strdup_printf("%s/stat", proc_dir), "r");
                if (stat_file != NULL) {
                    fscanf(stat_file, "%*s %ms %*s %*s %*s %ms", &name, &state);
                    fclose(stat_file);
                }
                FILE *status_file = fopen(g_strdup_printf("%s/status", proc_dir), "r");
                if (status_file != NULL) {
                    char line[256];
                    while (fgets(line, sizeof(line), status_file)) {
                        if (strncmp(line, "VmSize:", 7) == 0) {
                            sscanf(line, "%*s %ms", &memory);
                            break;
                        }
                    }
                    fclose(status_file);
                }

                // 添加到列表视图
                GtkTreeIter iter;
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, pid, 1, name, 2, state, 3, memory, -1);

                g_free(pid);
                g_free(proc_dir);
                g_free(name);
                g_free(state);
                g_free(memory);
            }
        }
        closedir(dir);
    }
}

GtkWidget *create_process_list_view() {
    GtkWidget *treeview;
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    update_process_list(store);

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    renderer = gtk_cell_renderer_text_new();

    column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes("State", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes("Memory", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    return treeview;
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    process_list_view = create_process_list_view();
    gtk_box_pack_start(GTK_BOX(vbox), process_list_view, TRUE, TRUE, 0);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
