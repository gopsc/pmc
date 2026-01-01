package org.cancerai.pmc_backend.entility;

public class ProcessEntility {
    private String pid;
    private String status;
    private String cmd;

    public ProcessEntility(String pid, String status, String cmd) {
        this.pid = pid;
        this.status = status;
        this.cmd = cmd;
    }

    public String getPid() {
        return pid;
    }

    public void setPid(String pid) {
        this.pid = pid;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public String getCmd() {
        return cmd;
    }

    public void setCmd(String cmd) {
        this.cmd = cmd;
    }
}
