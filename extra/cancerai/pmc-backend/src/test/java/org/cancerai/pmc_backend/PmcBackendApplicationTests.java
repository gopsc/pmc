package org.cancerai.pmc_backend;

import org.junit.jupiter.api.Test;
import org.springframework.boot.test.context.SpringBootTest;

@SpringBootTest
class PmcBackendApplicationTests {

	@Test
	void contextLoads() {
		StringBuilder stringBuilder = new StringBuilder();

		stringBuilder.append('a');
		System.out.println(stringBuilder);

		stringBuilder.setLength(0);
		stringBuilder.append('b');
		System.out.println(stringBuilder);
	}

}
