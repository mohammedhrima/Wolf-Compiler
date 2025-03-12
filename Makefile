# Variables
COMPOSE = docker-compose -f docker/docker-compose.yml
PROJECT_NAME = $(shell basename $(PWD))

# Targets
all: build up

build:
	@echo "Building Docker containers..."
	$(COMPOSE) build

up:
	@echo "Starting Docker containers..."
	$(COMPOSE) up -d

down:
	@echo "Stopping Docker containers..."
	$(COMPOSE) down

clean: down
	@echo "Removing Docker volumes..."
	docker volume rm -f $$(docker volume ls -q --filter name=$(PROJECT_NAME)_*)

fclean: down
	@echo "Cleaning Docker system..."
	docker system prune -af
	docker volume prune -f
	docker network prune -f

logs:
	@echo "Showing logs..."
	$(COMPOSE) logs -f

re: clean all

.PHONY: all build up down clean fclean logs re