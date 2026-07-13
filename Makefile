.PHONY: doctor setup build start stop restart clean logs status

doctor:
	@bash autobuild/docker.sh doctor

setup:
	@bash autobuild/docker.sh setup

build:
	@bash autobuild/docker.sh build

start:
	@bash autobuild/docker.sh compose

stop:
	@docker compose down

restart:
	@docker compose down
	@bash autobuild/docker.sh compose

clean:
	@bash autobuild/docker.sh clean

logs:
	@docker compose logs -f app

status:
	@echo "=== 容器状态 ==="
	@docker compose ps
	@echo ""
	@echo "=== 数据卷 ==="
	@docker volume ls --filter "name=shortener-" --format "table {{.Name}}\t{{.Driver}}"
