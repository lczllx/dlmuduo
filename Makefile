.PHONY: start stop restart build clean logs status

start:
	@bash start.sh --build

stop:
	@bash start.sh --stop

restart:
	@bash start.sh --stop
	@bash start.sh --build

build:
	docker build -t shortener-app:latest -f Dockerfile .

clean:
	@bash start.sh --stop
	docker volume rm shortener-mysql-data 2>/dev/null || true
	docker rmi shortener-app:latest 2>/dev/null || true

logs:
	docker logs -f shortener-app

status:
	@echo "=== 容器状态 ==="
	@docker ps --filter "name=shortener-" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
	@echo ""
	@echo "=== 数据卷 ==="
	@docker volume ls --filter "name=shortener-" --format "table {{.Name}}\t{{.Driver}}"
