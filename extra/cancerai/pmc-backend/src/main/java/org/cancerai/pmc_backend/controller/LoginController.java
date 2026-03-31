package org.cancerai.pmc_backend.controller;

import jakarta.servlet.http.HttpServletResponse;
import org.cancerai.pmc_backend.entility.User;
import org.cancerai.pmc_backend.service.LoginService;
import org.cancerai.pmc_backend.utils.EncryptUtils;
import org.cancerai.pmc_backend.vo.Result;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

@CrossOrigin(origins = "*")
@RestController
@RequestMapping("/api")
public class LoginController {
    @Autowired
    private LoginService loginService;

    @PostMapping("/login")
    public Result login(
            @RequestBody User user,
            HttpServletResponse response
            ) {
        String name = user.getName();
        String password = EncryptUtils.SHA256Encrypt(user.getPassword());

        return loginService.verifyUser(name, password, response);
    }

}
