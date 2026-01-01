package org.cancerai.pmc_backend.exception;

import org.cancerai.pmc_backend.vo.Result;
import org.springframework.http.HttpStatus;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.HttpRequestMethodNotSupportedException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.web.servlet.resource.NoResourceFoundException;

@RestControllerAdvice
public class GlobalExceptionHandler {
//    400
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    @ExceptionHandler(HttpMessageNotReadableException.class)
    public static Result JWTSignature(HttpMessageNotReadableException e) {
        e.printStackTrace();
        return Result.error("请求参数无效");
    }

    //    401
    @ResponseStatus(HttpStatus.METHOD_NOT_ALLOWED)
    @ExceptionHandler(HttpRequestMethodNotSupportedException.class)
    public static Result requestMethodNotSupport(HttpRequestMethodNotSupportedException e) {
        e.printStackTrace();
        return Result.error("请求方法错误");
    }

    //    404
    @ResponseStatus(HttpStatus.NOT_FOUND)
    @ExceptionHandler(NoResourceFoundException.class)
    public static Result ResourceNotFound(NoResourceFoundException e) {
        e.printStackTrace();
        return Result.error("未找到静态资源");
    }

//    500
    @ResponseStatus(HttpStatus.INTERNAL_SERVER_ERROR)
    @ExceptionHandler(Exception.class)
    public static Result exception(Exception e) {
        e.printStackTrace();
        return Result.error("服务器内部错误");
    }
}
