import pygame
import random

# VENTANA Y CELDAS
WIDTH, HEIGHT = 800, 600
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

# PARTICULAS ACTIVAS
active_sand = set()
active_water = set()
active_wood = set()
active_fire = {}
active_smoke = set()

# PYGAME
pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Falling Sand Simulator")
clock = pygame.time.Clock()

# GRID
grid = [[EMPTY for _ in range(ROWS)] for _ in range(COLS)]

def draw():
    screen.fill(COLOR_BG)

    for x,y in active_sand:
        pygame.draw.rect(screen, COLOR_SAND, (x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE))

    for x,y in active_water:
        pygame.draw.rect(screen, COLOR_WATER, (x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE))

    for x,y in active_wood:
        pygame.draw.rect(screen, COLOR_WOOD, (x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE))

    for x,y in active_fire:
        pygame.draw.rect(screen, COLOR_FIRE, (x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE))

    for x,y in active_smoke:
        pygame.draw.rect(screen, COLOR_SMOKE, (x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE, CELL_SIZE))

def in_bounds(x, y):
    return 0 <= x < COLS and 0 <= y < ROWS

def swap_particles(x1, y1, x2, y2):
    grid[x1][y1], grid[x2][y2] = grid[x2][y2], grid[x1][y1]
    for s in [active_sand, active_water, active_smoke, active_wood]:
        if (x1,y1) in s:
            s.discard((x1,y1)); s.add((x2,y2))
        elif (x2,y2) in s:
            s.discard((x2,y2)); s.add((x1,y1))
    if (x1,y1) in active_fire:
        active_fire[(x2,y2)] = active_fire.pop((x1,y1))
    elif (x2,y2) in active_fire:
        active_fire[(x1,y1)] = active_fire.pop((x2,y2))

def update():
    
# --- Arena ---
    for x, y in list(active_sand):
        if not in_bounds(x, y): continue
        below = (x, y+1)
        if in_bounds(*below) and (grid[below[0]][below[1]] in [EMPTY, WATER]):
            swap_particles(x, y, *below)
        else:
            dx = random.choice([-1,1])
            diag = (x+dx, y+1)
            if in_bounds(*diag) and grid[diag[0]][diag[1]] in [EMPTY, WATER]:
                swap_particles(x, y, *diag)

    # --- Agua ---
    for x, y in list(active_water):
        if not in_bounds(x, y): continue
        below = (x, y+1)
        if in_bounds(*below) and grid[below[0]][below[1]] == EMPTY:
            swap_particles(x, y, *below)
        else:
            dx = random.choice([-1,1])
            side = (x+dx, y)
            if in_bounds(*side) and grid[side[0]][side[1]] == EMPTY:
                swap_particles(x, y, *side)

    # --- Fuego ---
    for x, y in list(active_fire):
        if not in_bounds(x, y): continue
        # Quemar madera adyacente
        for dx, dy in [(-1,0),(1,0),(0,-1),(0,1),(1,1),(-1,-1),(-1,1),(1,-1)]:
            nx, ny = x+dx, y+dy
            if in_bounds(nx, ny) and grid[nx][ny] == WOOD:
                add_particle(nx, ny, FIRE)
        # Vida útil
        active_fire[(x,y)] -= 1
        if active_fire[(x,y)] <= 0 or random.random() < 0.02:
            remove_particle(x, y)
            above = (x, y-1)
            if in_bounds(*above) and grid[above[0]][above[1]] == EMPTY:
                add_particle(*above, SMOKE)

    # --- Humo ---
    for x, y in list(active_smoke):
        if not in_bounds(x, y): continue
        above = (x, y-1)
        if in_bounds(*above) and grid[above[0]][above[1]] == EMPTY:
            swap_particles(x, y, *above)
        else:
            if random.random() < 0.01:
                remove_particle(x, y)
            else:
                dx = random.choice([-1,1])
                side = (x+dx, y)
                if in_bounds(*side) and grid[side[0]][side[1]] == EMPTY:
                    swap_particles(x, y, *side)



def add_particle_area(x, y, p_type, size=2):
    for dx in range(-size, size+1):
        for dy in range(-size, size+1):
            if dx*dx + dy*dy <= size*size:
                add_particle(x+dx, y+dy, p_type)


def add_particle(x, y, p_type):
    if not in_bounds(x, y):
        return;

    remove_particle(x,y)
    grid[x][y] = p_type
    if p_type == SAND:
        active_sand.add((x, y))
    elif p_type == WATER:
        active_water.add((x, y))
    elif p_type == FIRE:
        active_fire[(x, y)] = random.randint(30, 100)  # vida útil en frames
    elif p_type == SMOKE:
        active_smoke.add((x, y))
    elif p_type == WOOD:
        active_wood.add((x,y))

def remove_particle(x, y):
    p = grid[x][y]
    grid[x][y] = EMPTY
    if p == SAND: active_sand.discard((x, y))
    elif p == WATER: active_water.discard((x, y))
    elif p == FIRE: active_fire.pop((x, y), None)
    elif p == SMOKE: active_smoke.discard((x, y))
    elif p == WOOD: active_wood.discard((x,y))

def main():
    running = True
    current_particle = SAND
    MOUSE_AREA = 2

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
            elif event.type == pygame.MOUSEWHEEL:
                MOUSE_AREA = max(1, min(MOUSE_AREA + event.y, 30))


        mouse_pressed = pygame.mouse.get_pressed()
        if mouse_pressed[0]:
            mx, my = pygame.mouse.get_pos()
            gx, gy = mx // CELL_SIZE, my // CELL_SIZE
            add_particle_area(gx, gy, current_particle, MOUSE_AREA)

        update()
        draw()
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()

if __name__ == "__main__":
    main()
