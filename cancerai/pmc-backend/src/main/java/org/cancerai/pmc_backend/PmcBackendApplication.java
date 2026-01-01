package org.cancerai.pmc_backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.web.servlet.ServletComponentScan;

@ServletComponentScan
@SpringBootApplication
public class PmcBackendApplication {

    public static void main(String[] args) {
        SpringApplication.run(PmcBackendApplication.class, args);
    }

}
