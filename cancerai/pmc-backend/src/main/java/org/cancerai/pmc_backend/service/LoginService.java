package org.cancerai.pmc_backend.service;

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import jakarta.servlet.http.HttpServletResponse;
import org.cancerai.pmc_backend.entility.Note;
import org.cancerai.pmc_backend.utils.DNFSUtils;
import org.cancerai.pmc_backend.utils.JWTUtils;
import org.cancerai.pmc_backend.vo.Result;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

@Service
public class LoginService {
    public static Result verifyUser(String name, String password, HttpServletResponse response) {
        ArrayList<HashMap<String, String>> allUsersInfo = getAllUsersInfo();
        boolean isExist = false;

        for (HashMap<String, String> u : allUsersInfo) {
            if (name.equals(u.get("name"))
                    && password.equals(u.get("password"))
            ) {
                isExist = true;
            }
        }

        if (isExist) {
            return Result.success(
                    "登陆成功",
                    JWTUtils.genJWT(name, password));
        } else {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return Result.error("账号或密码错误");
        }
    }

    private static ArrayList<HashMap<String, String>> getAllUsersInfo() {
        Gson gson = new Gson();
//        StringBuilder stringBuilder = new StringBuilder();
        ArrayList<String> userList = new ArrayList<>();
        ArrayList<Note> re = DNFSUtils.getNodes(0);
        String jsonString = gson.toJson(re);
        JsonArray jsonArray = gson.fromJson(jsonString, JsonArray.class);

        for (JsonElement jsonElement : jsonArray) {
//            To json
            JsonObject jsonObject = jsonElement.getAsJsonObject();
//            The value is \" + VALUE + \"
            String titleString = jsonObject.get("title").toString();
            String titleValue = titleString.substring(1, titleString.length() - 1);
            if ("Users".equals(titleValue)) {
                String contentString = jsonObject.get("content").toString();
                String contentValue = contentString.substring(1, contentString.length() - 1);

                String[] contentSplit = contentValue.split("\\\\n");
                userList.addAll(Arrays.asList(contentSplit));
            }
        }

        return DNFSUtils.parseUserInfo(userList);
    }
}
