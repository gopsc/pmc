package org.cancerai.pmc_backend.vo;

public class Result {
    private String message;
    private Object data;

    private Result(String message, Object data) {
        this.message = message;
        this.data = data;
    }

    public static Result success(String message) {
        return new Result(message, null);
    }

    public static Result success(String message, Object data) {
        return new Result(message, data);
    }

    public static Result error(String message) {
        return new Result(message, null);
    }

    public static Result error(String message, Object data) {
        return new Result(message, data);
    }

    //    getter and setter
    public String getMessage() {
        return message;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public Object getData() {
        return data;
    }

    public void setData(Object data) {
        this.data = data;
    }
}
