void setUseFanartMode(bool useFanart);
void setThumbnailSize(int size);
std::shared_ptr<VideoItem> getVideoItem() const { return m_video; }
void updateThumbnail();

protected:
void paintEvent(QPaintEvent *event) override; 