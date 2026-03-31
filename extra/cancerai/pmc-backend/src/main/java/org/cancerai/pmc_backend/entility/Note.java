package org.cancerai.pmc_backend.entility;

public class Note {
    private String content;
    private int id;
    private int pid;
    private String title;

    public Note(String content, int id, int pid, String title) {
        this.content = content;
        this.id = id;
        this.pid = pid;
        this.title = title;
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public int getPid() {
        return pid;
    }

    public void setPid(int pid) {
        this.pid = pid;
    }

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    @Override
    public String toString() {
        return "Note{" +
                "content='" + content + '\'' +
                ", id=" + id +
                ", pid=" + pid +
                ", title='" + title + '\'' +
                '}';
    }
}
