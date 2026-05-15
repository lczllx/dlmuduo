.PHONY: docker-build docker-up docker-down docker-restart docker-clean docker-rebuild docker-logs docker-status docker-all

# 自动检测 docker compose v2 插件，不存在则回退到 docker-compose v1
COMPOSE := $(shell docker compose version >/dev/null 2>&1 && echo "docker compose" || echo "docker-compose")

docker-build:
	$(COMPOSE) build

docker-up:
	$(COMPOSE) up -d

docker-down:
	$(COMPOSE) down

docker-restart:
	$(COMPOSE) down && $(COMPOSE) up -d

docker-clean:
	$(COMPOSE) down -v --rmi local

docker-rebuild:
	$(COMPOSE) build --no-cache && $(COMPOSE) up -d

docker-logs:
	$(COMPOSE) logs -f app

docker-status:
	$(COMPOSE) ps

docker-all: docker-build docker-up
	@echo "=== 服务已启动 ==="
	$(COMPOSE) ps
