library(tidyverse)
library(colorspace)
library(phenovisr)

# Puts the description of all the masks in a tibble. Originally this description included masks
# from 'data/d0', but to keep things fast I'll just use the community mask.
df.masks <- tibble(Mask.Filename=list.files("masks", recursive=TRUE, pattern="jpg", full.names=TRUE)) %>%
  separate(Mask.Filename, sep="/", into=c("directory", "Mask"), remove=FALSE) %>%
  mutate(Mask = gsub(".jpg", "", Mask)) %>%
  mutate(Mask = gsub("-", "_", Mask)) %>%
  select(-directory) %>%
  filter(Mask == 'dc_TKY_y18_d_mask_section_1')

# Puts the description of all the images in another tibble
df.pen <- tibble(Picture.Filename=list.files("PEN", pattern="jpg", recursive=TRUE, full.names=TRUE)) %>%
  separate(Picture.Filename, sep="/", into=c("directory", "Picture"), remove=FALSE) %>% 
  select(-directory) %>%
  mutate(Picture = gsub(".jpg", "", Picture))

# Now let's extract some data from the image name
df.pen <- df.pen %>% 
  separate(Picture, sep='_', into=c('A', 'Year', 'Day', 'Time', 'Dataset', 'Camera', 'B'), remove=FALSE) %>%
  select(-A, -B) %>%
  mutate(Time = gsub('\\+.*', '', Time)) %>%
  mutate(Time = as.factor(Time))

# Here we select the pictures that we want to consider for this analysis
df.pen.selected <- df.pen %>%
  filter(Dataset == 'AHS') %>%
  filter(Camera == 't24') %>%
  filter(Year == 2017) %>%
  filter(Time == 1200) %>%
  select(Picture.Filename, Picture)

# Joining pictures to masks...
df.pen.selected.masks <- df.pen.selected %>%
  mutate(dummy = TRUE) %>%
  left_join(df.masks %>% mutate(dummy = TRUE), by = c('dummy')) %>%
  select(-dummy)

# Calculate the histogram for all the images (warning: this can take a potentially long time)
gethist <- function(df, grain=10) {
  mask <- df %>% slice(1) %>% pull(Mask.Filename)
  phenovis_read_mask(mask)
  phenovis_get_HSV_double_histogram(phenovis_H(), df %>% pull(Picture.Filename), grain) %>%
    filter(H != 0) %>%
    as_tibble()
}
df.histograms <- df.pen.selected.masks %>%
  group_by(Mask) %>%
  do(gethist(.)) %>%
  ungroup()


# Tidy the data
df.histograms.tidy <- df.histograms %>%
  rename(H.count = Count) %>%
  # V histogram is spread, let's gather it
  gather(V, V.count, -Mask, -Name, -Width, -Height, -Pixels, -H, -H.count) %>%
  # Remove the V, convert to integer
  mutate(V = as.integer(gsub('V', '', V))) %>%
  # Map to [0,1], convert to double
  mutate(V = as.numeric(round(V/9, 2))) %>%
  # Cosmetics
  separate(Picture, sep='_', into=c('A', 'Year', 'Day', 'Time', 'Dataset', 'Camera', 'B'), remove=FALSE) %>%
  select(-A, -Dataset, -Camera, -B) %>%
  mutate(Time = gsub('\\+.*', '', Time)) %>%
  mutate(Time = as.factor(Time))
