package org.cancerai.pmc_backend.filter;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import io.jsonwebtoken.ExpiredJwtException;
import io.jsonwebtoken.MalformedJwtException;
import jakarta.servlet.*;
import jakarta.servlet.annotation.WebFilter;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.cancerai.pmc_backend.utils.HttpResponseText;
import org.cancerai.pmc_backend.utils.JWTUtils;
import org.cancerai.pmc_backend.vo.Result;
import org.springframework.core.annotation.Order;

import java.io.IOException;
import java.io.PrintWriter;

@WebFilter(urlPatterns = "/api/*")
@Order(1)
public class LoginFilter implements Filter {
    final String[] EXCLUDE_URL_LIST = {
            "/api/login"
    };

    @Override
    public void doFilter(ServletRequest servletRequest, ServletResponse servletResponse, FilterChain filterChain) throws IOException, ServletException {
        HttpServletRequest request = (HttpServletRequest) servletRequest;
        HttpServletResponse response = (HttpServletResponse) servletResponse;
//        set content security policy
        response.setHeader("Content-Security-Policy", "default-src *");

//        exclude url
        String uri = request.getRequestURI();
        for (String item : EXCLUDE_URL_LIST) {
            if (item.equals(uri)) {
                filterChain.doFilter(servletRequest, servletResponse);
                return;
            }
        }
//        Allow OPTION method
        if ("OPTIONS".equals(request.getMethod())) {
            filterChain.doFilter(servletRequest, servletResponse);
            return;
        }

        String token = request.getHeader("token");
        if (token == null || token.isEmpty()) {
//            set response header
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            response.setContentType("application/json");

//            construct body
            Gson gson = new GsonBuilder().serializeNulls().create();
            String responseBody = gson.toJson(Result.error(HttpResponseText.UNAUTHORIZED)).toString();
//            set response body
            PrintWriter writer = response.getWriter();
            writer.write(responseBody);
            writer.flush();
            return;
        }

        try {
            JWTUtils.parseJWT(token);
        } catch (MalformedJwtException e) {
//            set response body
            Gson gson = new GsonBuilder().serializeNulls().create();
            String body = gson.toJson(Result.error(HttpResponseText.JWT_MALFORMED));

            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            PrintWriter writer = response.getWriter();
            writer.write(body);
            writer.flush();
            return;
        } catch (ExpiredJwtException e) {
//            set response body
            Gson gson = new GsonBuilder().serializeNulls().create();
            String body = gson.toJson(Result.error(HttpResponseText.JWT_EXPIRED));

            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            PrintWriter writer = response.getWriter();
            writer.write(body);
            writer.flush();
            return;
        }

//        Allow
        filterChain.doFilter(servletRequest, servletResponse);
        response.setHeader("Content-Security-Policy", "default-src *");
    }
}
