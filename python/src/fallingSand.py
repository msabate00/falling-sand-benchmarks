import pygame
import random

# VENTANA Y CELDAS
WIDTH, HEIGHT = 400, 300
CELL_SIZE = 4
COLS, ROWS = WIDTH // CELL_SIZE, HEIGHT // CELL_SIZE

# COLORES
COLOR_BG = (0, 0, 0)
COLOR_SAND = (194, 178, 128)
COLOR_WATER = (64, 164, 223)
COLOR_WOOD = (101, 67, 33)
COLOR_FIRE = (255, 69, 0)
COLOR_SMOKE = (105, 105, 105)

# TIPOS DE PARTICULAS
EMPTY = 0
SAND = 1
WATER = 2
WOOD = 3
FIRE = 4
SMOKE = 5

# PYGAME
pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Falling Sand Simulator")
clock = pygame.time.Clock()

# GRID
grid = [[EMPTY for _ in range(ROWS)] for _ in range(COLS)]

def draw():
    screen.fill(COLOR_BG)
    for x in range(COLS):
        for y in range(ROWS):
            p = grid[x][y]
            if p == SAND:
                color = COLOR_SAND
            elif p == WATER:
                color = COLOR_WATER
            elif p == WOOD:
                color = COLOR_WOOD
            elif p == FIRE:
                color = COLOR_FIRE
            elif p == SMOKE:
                color = COLOR_SMOKE
            else:
                continue
            rect = (x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
            pygame.draw.rect(screen, color, rect)

def in_bounds(x, y):
    return 0 <= x < COLS and 0 <= y < ROWS

def swap(x1, y1, x2, y2):
    grid[x1][y1], grid[x2][y2] = grid[x2][y2], grid[x1][y1]

def update():
    
    for y in reversed(range(ROWS)):
        for x in range(COLS):
            p = grid[x][y]
            if p == SAND:
                if in_bounds(x, y+1):
                    below = grid[x][y+1]
                    if below == EMPTY or below == WATER:
                        swap(x, y, x, y+1)
                    else:
                        dx = random.choice([-1, 1])
                        if in_bounds(x + dx, y + 1):
                            diag = grid[x + dx][y + 1]
                            if diag == EMPTY or diag == WATER:
                                swap(x, y, x + dx, y + 1)

            elif p == WATER:
                if in_bounds(x, y+1):
                    if grid[x][y+1] == EMPTY:
                        swap(x, y, x, y+1)
                    else:
                        dx = random.choice([-1, 1])
                        if in_bounds(x + dx, y):
                            if grid[x + dx][y] == EMPTY:
                                swap(x, y, x + dx, y)

            elif p == FIRE:
                if in_bounds(x, y+1):
                    if grid[x][y+1] == WOOD:
                        grid[x][y+1] = FIRE
                for dx, dy in [(-1,0), (1,0), (0,-1), (0,1)]:
                    nx, ny = x + dx, y + dy
                    if in_bounds(nx, ny) and grid[nx][ny] == WOOD:
                        grid[nx][ny] = FIRE
                if random.random() < 0.02:
                    grid[x][y] = SMOKE
                    if in_bounds(x, y-1) and grid[x][y-1] == EMPTY:
                        grid[x][y-1] = SMOKE

            elif p == SMOKE:
                if in_bounds(x, y-1) and grid[x][y-1] == EMPTY:
                    swap(x, y, x, y-1)
                else:
                    if random.random() < 0.01:
                        grid[x][y] = EMPTY

def add_particle(x, y, p_type):
    if in_bounds(x, y):
        grid[x][y] = p_type

def main():
    running = True
    current_particle = SAND

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_1:
                    current_particle = SAND
                elif event.key == pygame.K_2:
                    current_particle = WATER
                elif event.key == pygame.K_3:
                    current_particle = WOOD
                elif event.key == pygame.K_4:
                    current_particle = FIRE
                elif event.key == pygame.K_5:
                    current_particle = SMOKE

        mouse_pressed = pygame.mouse.get_pressed()
        if mouse_pressed[0]:
            mx, my = pygame.mouse.get_pos()
            gx, gy = mx // CELL_SIZE, my // CELL_SIZE
            add_particle(gx, gy, current_particle)

        update()
        draw()
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()

if __name__ == "__main__":
    main()
