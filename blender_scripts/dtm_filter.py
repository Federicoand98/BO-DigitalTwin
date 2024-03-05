import os


in_folder = ""

count = 0

for filename in os.listdir(in_folder):
    if not filename.startswith('32_') or not filename.endswith('.ASC'):
        continue

    # Estrai i numeri dal nome del file
    _, num1, num2 = filename.split('_')
    num2 = num2.split('.')[0]  # Rimuovi l'estensione del file

    num1 = int(num1)
    num2 = int(num2)

    if num1 >= 685000 and num1 <= 687000 and num2 >= 4928000 and num2 <= 4930500:
        if os.path.exists(os.path.join(in_folder, filename)):
            os.remove(os.path.join(in_folder, filename))

    if num1 < 683000 or num1 > 689000 or num2 < 4926000 or num2 > 4932500:
        if os.path.exists(os.path.join(in_folder, filename)):
            os.remove(os.path.join(in_folder, filename))
