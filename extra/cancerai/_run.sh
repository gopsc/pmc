#!/bin/bash
cd "$(dirname "$0")" || exit
cd pmc-backend
exec mvn clean spring-boot:run
