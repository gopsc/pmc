package org.cancerai.pmc_backend.utils;

public class HttpResponseText {
//    success
    public final static String SUCCESS_TO_REQUEST = "Request OK";
    public final static String SUCCESS_TO_ADD_NOTE = "成功添加节点";
    public final static String SUCCESS_TO_DELETE_NOTE = "成功删除节点";
    public final static String SUCCESS_TO_UPDATE_NOTE = "成功更新节点";
//    error
    public final static String UNAUTHORIZED = "Cannot authorized identity, access denied";
    public final static String JWT_MALFORMED = "JWT无效";
    public final static String JWT_EXPIRED = "JWT过期";
    public final static String INTERNAL_SERVER_ERROR = "服务器内部错误";
}
